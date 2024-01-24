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

   Source File Name = clsIndexJob.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          2019/09/03  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/
#include "clsIndexJob.hpp"
#include "rtnCB.hpp"
#include "msgMessage.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "rtn.hpp"
#include "../bson/bson.h"
#include "clsMgr.hpp"

using namespace bson ;

namespace engine
{
   // normal thread use it
   _clsIndexJob::_clsIndexJob( RTN_JOB_TYPE type, UINT32 locationID,
                               clsIdxTask* pTask )
   {
      _type = type ;

      SDB_ASSERT( pTask, "pTask can't be null" ) ;

      _taskID = pTask->taskID() ;
      _locationID = locationID ;
      _mainTaskID = pTask->mainTaskID() ;

      if ( CLS_TASK_CREATE_IDX == pTask->taskType() )
      {
         clsCreateIdxTask* pTask1 = (clsCreateIdxTask*)pTask ;
         _indexObj = pTask1->indexDef() ;
         _hasSetIndexObj = TRUE ;
         _indexName = pTask1->indexName() ;
         _sortBufSize = pTask1->sortBufSize() ;
      }
      else if ( CLS_TASK_DROP_IDX == pTask->taskType() )
      {
         try
         {
            _indexObj = BSON( "" << pTask->indexName() ) ;
            _hasSetIndexObj = TRUE ;
         }
         catch( std::exception &e )
         {
            _hasSetIndexObj = FALSE ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
         _indexName = pTask->indexName() ;
         _sortBufSize = 0 ;
      }

      ossStrncpy( _clFullName, pTask->collectionName(),
                  DMS_COLLECTION_FULL_NAME_SZ ) ;
      _clFullName[DMS_COLLECTION_FULL_NAME_SZ] = 0 ;
      _clUniqID = pTask->clUniqueID() ;

      _threadMode = CLS_INDEX_NORMAL ;
      _retryLater = FALSE ;
      _checkTasks = FALSE ;
   }

   // rollback thread use it
   _clsIndexJob::_clsIndexJob( RTN_JOB_TYPE type,
                               dmsIdxTaskStatusPtr idxStatPtr,
                               CLS_INDEX_THREAD_MODE threadMod )
   {
      _type = type ;

      SDB_ASSERT( idxStatPtr.get(), "dmsIdxTaskStatus can't be null" ) ;
      _taskStatusPtr = idxStatPtr ;

      _taskID = _taskStatusPtr->taskID() ;
      _locationID = _taskStatusPtr->locationID() ;
      _mainTaskID = _taskStatusPtr->mainTaskID() ;

      if ( DMS_TASK_CREATE_IDX == _taskStatusPtr->taskType() )
      {
         _indexObj = _taskStatusPtr->indexDef() ;
         _hasSetIndexObj = TRUE ;
         _indexName = _taskStatusPtr->indexName() ;
         _sortBufSize = _taskStatusPtr->sortBufSize() ;
      }
      else if ( DMS_TASK_DROP_IDX == _taskStatusPtr->taskType() )
      {
         try
         {
            // DON'T use _taskStatusPtr->indexDef(), it may be empty
            _indexObj = BSON( "" << _taskStatusPtr->indexName() ) ;
            _hasSetIndexObj = TRUE ;
         }
         catch( std::exception &e )
         {
            _hasSetIndexObj = FALSE ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
         _indexName = _taskStatusPtr->indexName() ;
         _sortBufSize = 0 ;
      }

      _taskStatusPtr->collectionName( _clFullName, sizeof( _clFullName ) ) ;
      _clUniqID = _taskStatusPtr->clUniqueID() ;

      _threadMode = threadMod ;
      _retryLater = FALSE ;
      _checkTasks = FALSE ;
   }

   void _clsIndexJob::_onAttach()
   {
      _rtnIndexJob::_onAttach() ;

      // switch status if it is a rollback thread
      if ( _taskStatusPtr.get() )
      {
         if ( CLS_INDEX_ROLLBACK == _threadMode  )
         {
            _taskStatusPtr->setStatus( DMS_TASK_STATUS_ROLLBACK ) ;
         }
         else if ( CLS_INDEX_ROLLBACK_CANCEL == _threadMode  )
         {
            _taskStatusPtr->setStatus( DMS_TASK_STATUS_CANCELED ) ;
         }
         else if ( CLS_INDEX_RESTART == _threadMode  )
         {
            _taskStatusPtr->setStatus( DMS_TASK_STATUS_RUN ) ;
            _taskStatusPtr->setPauseReport( FALSE ) ;
            _taskStatusPtr->incRetryCnt() ;
         }
      }

      // start job to report task progress to catalog timely
      clsStartReportTaskInfoJob() ;
   }

   void _clsIndexJob::_onDetach()
   {
      clsCB *clsCB = sdbGetClsCB() ;

      // remove task at clsMgr and taskMgr
      clsCB->removeTask( _taskID ) ;
      clsCB->getTaskMgr()->removeTask( _locationID ) ;

      // add to task map to retry, should after removeTask
      if ( _retryLater )
      {
         if ( SDB_OK != clsCB->startIdxTaskCheck( _taskID ) )
         {
            PD_LOG( PDWARNING, "Failed to push task[%llu] to retry", _taskID ) ;
         }
      }
      else if ( _checkTasks &&
                NULL != _clFullName &&
                0 == clsCB->getTaskMgr()->taskCountByCL( _clFullName ) )
      {
         // start index task check if this is the last task of the collection
         INT32 tmpRC = clsCB->startIdxTaskCheckByCL( _clUniqID ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to push task for collection task, "
                    "rc: %d", tmpRC ) ;
         }
      }

      // update task status to FINISH in catalog
      clsCB->getTaskEvent()->signal() ;

      _rtnIndexJob::_onDetach() ;
   }

   // master node use the function
   INT32 _clsIndexJob::init ()
   {
      try
      {
         if ( !_hasSetIndexObj )
         {
            _indexObj = BSON( "" << _indexName ) ;
            _hasSetIndexObj = TRUE ;
         }

         if ( RTN_JOB_DROP_INDEX == _type )
         {
            _indexEle = _indexObj.getField( IXM_NAME_FIELD ) ;
            if ( _indexEle.eoo() )
            {
               _indexEle = _indexObj.firstElement() ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         return ossException2RC( &e ) ;
      }

      return _buildJobName() ;
   }

   INT32 _clsIndexJob::doit()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN writeDB = FALSE ;
      clsFreezingWindow* pWindow = sdbGetShardCB()->getFreezingWindow() ;
      pmdEDUCB* cb = eduCB() ;
      SDB_DMSCB* dmsCB = sdbGetDMSCB() ;
      SDB_DPSCB* dpsCB = sdbGetDPSCB() ;
      BOOLEAN nameIsOk = FALSE ;
      INT32 retryCnt = 0 ;
      BOOLEAN needRollback = FALSE ;

      // build task status ptr if not exist
      rc = _buildTaskStatus() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to build task status, rc: %d", rc ) ;
         goto error ;
      }

      // start catalog task, make task status from Ready to Running
      if ( CLS_INDEX_NORMAL == _threadMode || CLS_INDEX_RESTART == _threadMode )
      {
         rc = _startCatalogTask( _taskStatusPtr->taskID() ) ;
         if ( SDB_DMS_EOC == rc || SDB_CAT_TASK_NOTFOUND == rc )
         {
            _taskStatusPtr->setStatus2Finish( rc ) ;
            PD_LOG( PDWARNING,
                    "No need to execute task[%llu], not exist on catalog, "
                    "rc: %d", _taskStatusPtr->taskID(), rc ) ;
            rc = SDB_OK ;
            goto done ;
         }
         else if ( SDB_TASK_ALREADY_FINISHED == rc )
         {
            _taskStatusPtr->setStatus2Finish( rc ) ;
            PD_LOG( PDWARNING,
                    "No need to execute task[%llu], already finished on "
                    "catalog, rc: %d", _taskStatusPtr->taskID(), rc ) ;
            rc = SDB_OK ;
            // trigger task check to launch conflict tasks
            _checkTasks = TRUE ;
            goto done ;
         }
         else if ( SDB_TASK_HAS_CANCELED == rc || SDB_TASK_ROLLBACK == rc )
         {
            if ( RTN_JOB_CREATE_INDEX == _type )
            {
               _taskStatusPtr->setStatus( SDB_TASK_HAS_CANCELED == rc ?
                                          DMS_TASK_STATUS_CANCELED :
                                          DMS_TASK_STATUS_ROLLBACK ) ;
               needRollback = TRUE ;
            }
         }
         else if ( rc != SDB_OK )
         {
            PD_LOG( PDERROR, "Failed to start task[%llu] on catalog, rc: %d",
                    _taskStatusPtr->taskID(), rc ) ;
            // maybe catalog not exist / no primary, just retry
            _retryLater = TRUE ;
            goto error ;
         }
      }

   retry:
      // wait for operation which register this cl
      cb->setCurProcessName( _clFullName ) ;
      cb->writingDB( TRUE ) ;
      writeDB = TRUE ;

      rc = pWindow->waitForOpr( _clFullName, cb, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR,
                 "Failed to wait freezing window for collection[%s], rc: %d",
                 _clFullName, rc ) ;
         _taskStatusPtr->setStatus2Finish( rc ) ;
         goto error ;
      }

      // check collection name by unique id, in case collection/space rename
      rc = _checkAndFixCLNameByID( nameIsOk ) ;
      if ( rc )
      {
         PD_LOG( PDERROR,
                 "Failed to check collection name[%s] by unique id[%llu], "
                 "rc: %d", _clFullName, _clUniqID, rc ) ;
         _taskStatusPtr->setStatus2Finish( rc ) ;
         goto error ;
      }
      if ( !nameIsOk )
      {
         // _clFullName is wrong, just retry
         cb->writingDB( FALSE ) ;
         writeDB = FALSE ;
         if ( retryCnt++ < 5 )
         {
            cb->clearProcessInfo() ;
            goto retry ;
         }
         else
         {
            _retryLater = TRUE ;
            goto error ;
         }
      }

      // do rollback
      if ( needRollback )
      {
         BSONObj index ;
         try
         {
            index = BSON( "" << _indexName.c_str() ) ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            _taskStatusPtr->setStatus2Finish( rc ) ;
            needRollback = FALSE ;
            PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
         }

         if ( UTIL_IS_VALID_CLUNIQUEID( _clUniqID ) )
         {
            rc = rtnDropIndexCommand( _clUniqID, index.firstElement(),
                                      cb, dmsCB, dpsCB ) ;
         }
         else
         {
            rc = rtnDropIndexCommand( _clFullName, index.firstElement(),
                                      cb, dmsCB, dpsCB ) ;
         }
         _taskStatusPtr->setStatus2Finish( rc ) ;
         needRollback = FALSE ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop index[%s] "
                      "for collection[%s:%llu], rc: %d",
                      _indexName.c_str(), _clFullName, _clUniqID, rc ) ;
         goto done ;
      }

      // do it
      rc = _rtnIndexJob::doit() ;
      // result code and finish status has been set at doit()
      PD_RC_CHECK( rc, ( _retryLater ? PDWARNING : PDERROR ),
                   "Failed to do index job, rc: %d", rc ) ;

   done:
      if ( writeDB )
      {
         cb->writingDB( FALSE ) ;
      }
      cb->clearProcessInfo() ;
      return rc ;
   error:
      if ( needRollback )
      {
         _taskStatusPtr->setStatus2Finish( rc ) ;
      }
      goto done ;
   }

   INT32 _clsIndexJob::_buildTaskStatus()
   {
      INT32 rc = SDB_OK ;

      if ( !_taskStatusPtr.get() )
      {
         DMS_TASK_TYPE statType = DMS_TASK_UNKNOWN ;
         dmsTaskStatusMgr* pStatMgr = sdbGetRTNCB()->getTaskStatusMgr() ;

         if ( RTN_JOB_CREATE_INDEX == _type )
         {
            statType = DMS_TASK_CREATE_IDX ;
         }
         else if ( RTN_JOB_DROP_INDEX == _type )
         {
            statType = DMS_TASK_DROP_IDX ;
         }

         rc = pStatMgr->createIdxItem( statType, _taskStatusPtr,
                                       _taskID, _locationID, _mainTaskID ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to create task status, rc: %d",
                      rc ) ;

         rc = _taskStatusPtr->init( _clFullName, _indexObj, _sortBufSize,
                                    _clUniqID ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to initialize task status, rc: %d",
                      rc ) ;
      }

   done:
      return rc ;
   error:
      _retryLater = TRUE ;
      goto done ;
   }

   BOOLEAN _clsIndexJob::_isCLNameExist()
   {
      INT32 rc                   = SDB_OK ;
      dmsStorageUnitID suID      = DMS_INVALID_SUID ;
      dmsStorageUnit *su         = NULL ;
      SDB_DMSCB *pDmsCB          = sdbGetDMSCB() ;
      dmsMBContext *pMBContext   = NULL ;
      const CHAR *clShortName    = NULL ;
      BOOLEAN exist              = FALSE ;

      rc = rtnResolveCollectionNameAndLock( _clFullName, pDmsCB, &su,
                                            &clShortName, suID, SHARED ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to loop up su by collection name[%s], rc: %d",
                   _clFullName, rc ) ;

      rc = su->data()->getMBContext( &pMBContext, clShortName ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get mb context by collection name[%s], rc: %d",
                   clShortName, rc ) ;

      exist = TRUE ;

   done:
      if ( pMBContext )
      {
         su->data()->releaseMBContext( pMBContext ) ;
         pMBContext = NULL ;
      }
      if ( suID != DMS_INVALID_SUID )
      {
         pDmsCB->suUnlock( suID, SHARED ) ;
         suID = DMS_INVALID_SUID ;
         su = NULL ;
      }
      return exist ;
   error:
      goto done ;
   }

   INT32 _clsIndexJob::_checkAndFixCLNameByID( BOOLEAN& isOk )
   {
      INT32 rc                   = SDB_OK ;
      utilCSUniqueID csUniqueID  = utilGetCSUniqueID( _clUniqID ) ;
      dmsStorageUnitID suID      = DMS_INVALID_SUID ;
      dmsStorageUnit *su         = NULL ;
      SDB_DMSCB *pDmsCB          = sdbGetDMSCB() ;
      dmsMBContext *pMBContext   = NULL ;
      CHAR clNameInData[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;

      isOk = FALSE ;

      rc = pDmsCB->idToSUAndLock( csUniqueID, suID, &su, SHARED ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to loop up su by cs unique id[%u], rc: %d",
                   csUniqueID, rc ) ;

      rc = su->data()->getMBContextByID( &pMBContext, _clUniqID, SHARED ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get mb context by cl unique id[%llu], rc: %d",
                   _clUniqID, rc ) ;

      ossSnprintf( clNameInData, sizeof( clNameInData ),
                   "%s.%s", su->CSName(), pMBContext->mb()->_collectionName ) ;

      if ( 0 == ossStrcmp( clNameInData, _clFullName ) )
      {
         isOk = TRUE ;
      }
      else
      {
         isOk = FALSE ;
         PD_LOG( PDWARNING, "Collection name[%s] doesn't match unique id[%llu],"
                 " fix collection name to [%s]",
                 _clFullName, _clUniqID, clNameInData ) ;

         ossStrncpy( _clFullName, clNameInData, DMS_COLLECTION_FULL_NAME_SZ ) ;
         _clFullName[ DMS_COLLECTION_FULL_NAME_SZ ] = 0 ;

         _taskStatusPtr->collectionRename( clNameInData ) ;
      }

   done:
      if ( pMBContext )
      {
         su->data()->releaseMBContext( pMBContext ) ;
         pMBContext = NULL ;
      }
      if ( suID != DMS_INVALID_SUID )
      {
         pDmsCB->suUnlock( suID, SHARED ) ;
         suID = DMS_INVALID_SUID ;
         su = NULL ;
      }
      if ( ( SDB_DMS_CS_NOTEXIST == rc ||
             SDB_DMS_NOTEXIST == rc ) && _isCLNameExist() )
      {
         // If cl unique id not exist, but cl name exist, means cl unique id is
         // wrong, we just ignore error
         PD_LOG( PDWARNING, "Collection[%s]'s unqiue id isn't equal to task's "
                 "collection unique id[%llu]", _clFullName, _clUniqID ) ;
         rc = SDB_OK ;
         isOk = TRUE ;
         _clUniqID = UTIL_UNIQUEID_NULL ;

      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsIndexJob::_startCatalogTask( UINT64 taskID )
   {
      INT32 rc = SDB_OK ;
      CHAR *pBuff = NULL ;
      INT32 buffSize = 0 ;
      MsgHeader *pRcvMsg = NULL ;
      INT32 resultCode = SDB_OK ;
      shardCB* pShardCB = sdbGetShardCB() ;
      BSONObj matcher ;

      // build message
      try
      {
         matcher = BSON( FIELD_NAME_TASKID << (INT64)taskID <<
                         FIELD_NAME_GROUPNAME << pmdGetKRCB()->getGroupName() ) ;
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      rc = msgBuildQueryMsg ( &pBuff, &buffSize, "", 0, 0, 0, -1,
                              &matcher, NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build query message, rc: %d",
                   rc ) ;

      ((MsgHeader*)pBuff)->opCode = MSG_CAT_TASK_START_REQ ;

   retry:
      // send message
      rc = pShardCB->syncSend( (MsgHeader*)pBuff, CATALOG_GROUPID, TRUE,
                               &pRcvMsg ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to send message, rc: %d",
                   rc ) ;

      resultCode = ((MsgOpReply*)pRcvMsg)->flags ;
      PD_LOG ( PDDEBUG,
               "Receive MSG_CAT_TASK_START_REQ response[taskID: %llu, rc: %d]",
               taskID, resultCode ) ;

      /// if catalog reply with error code
      if ( SDB_CLS_NOT_PRIMARY      == resultCode ||
           SDB_CLS_WAIT_SYNC_FAILED == resultCode )
      {
         if ( SDB_OK != pShardCB->updatePrimaryByReply( pRcvMsg ) )
         {
            pShardCB->updateCatGroup() ;
         }
         PD_LOG( PDERROR, "Failed to start task[%llu] on catalog, rc: %d",
                 taskID, resultCode ) ;

         SDB_OSS_FREE ( (CHAR*)pRcvMsg ) ;
         pRcvMsg = NULL ;
         ossSleep( OSS_ONE_SEC ) ;
         goto retry ;
      }
      else if ( SDB_OK != resultCode )
      {
         rc = resultCode ;
      }

   done:
      if ( pBuff )
      {
         SDB_OSS_FREE ( pBuff ) ;
         pBuff = NULL ;
      }
      if ( pRcvMsg )
      {
         SDB_OSS_FREE ( (CHAR*)pRcvMsg ) ;
         pRcvMsg = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsIndexJob::_onDoit( INT32 resultCode )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _taskStatusPtr.get(), "taskStatusPtr can't be null" ) ;
      PD_CHECK( _taskStatusPtr.get(), SDB_SYS, error,
                PDERROR, "taskStatusPtr can't be null" ) ;

      /// process this collection's main-collection
      if ( _taskStatusPtr->mainTaskID() != CLS_INVALID_TASKID &&
           SDB_OK == resultCode )
      {
         CHAR mainCLName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
         shardCB* pShard = sdbGetShardCB() ;
         clsCatalogSet* pCatSet = NULL ;

         rc = pShard->getAndLockCataSet( _clFullName, &pCatSet ) ;
         if ( SDB_OK == rc && pCatSet )
         {
            ossStrncpy( mainCLName, pCatSet->getMainCLName().c_str(),
                        DMS_COLLECTION_FULL_NAME_SZ ) ;
         }
         pShard->unlockCataSet( pCatSet ) ;

         if ( mainCLName[0] != 0 )
         {
            // Clear cached main-collection plans. Cached sub-collection plans
            // are cleared inside create/drop index of sub-collections
            sdbGetRTNCB()->getAPM()->invalidateCLPlans( mainCLName ) ;
            // Tell secondary nodes to clear cached main-collection plans
            sdbGetClsCB()->invalidatePlan( mainCLName ) ;
         }
      }

      /// process global index
      if ( _taskStatusPtr->isGlobalIdx() )
      {
         const CHAR* globalIdxCLName = NULL ;
         utilCLUniqueID globalIdxCLUniqID = UTIL_UNIQUEID_NULL ;
         rc = _taskStatusPtr->globalIdxCL( globalIdxCLName,
                                           globalIdxCLUniqID ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get global index collection info, rc: %d",
                      rc ) ;

         IRemoteOperator *pRemoteOpr = NULL ;
         rc = eduCB()->getOrCreateRemoteOperator( &pRemoteOpr ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get remote operator, rc: %d",
                      rc ) ;

         if ( DMS_TASK_CREATE_IDX == _taskStatusPtr->taskType() )
         {
            if ( resultCode != SDB_OK )
            {
               /// TODO drop global index collection
            }
         }
         else if ( DMS_TASK_DROP_IDX == _taskStatusPtr->taskType() )
         {
            /// wait all data nodes's index to be dropped
            const CHAR* indexName = _taskStatusPtr->indexName() ;
            rc = _waitIndexAllInvalid( pRemoteOpr, _clFullName, indexName ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check index[%s:%s] "
                         "invalid or not exist on all data nodes, rc: %d",
                         _clFullName, indexName, rc ) ;

            /// TODO drop global index collection
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsIndexJob::_waitIndexAllInvalid( IRemoteOperator* pRemoteOpr,
                                             const CHAR* collectionName,
                                             const CHAR* indexName )
   {
      /*
       * Check that this index is invalid or doesn't exist on all data nodes
       */
      INT32 rc = SDB_OK ;
      INT64 contextID = -1 ;
      BOOLEAN allInvalid = FALSE ;
      pmdEDUCB* cb = eduCB() ;
      SDB_RTNCB* rtnCB = sdbGetRTNCB() ;

      while ( TRUE )
      {
         rc = pRemoteOpr->snapshotIndexes( contextID, collectionName, indexName,
                                           TRUE ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to snapshot index by remote operator, rc: %d",
                      rc ) ;

         if ( contextID == -1 )
         {
            allInvalid = TRUE ;
         }
         else
         {
            rtnContextBuf buf ;
            rc = rtnGetMore( contextID, 1, buf, cb, rtnCB ) ;
            if ( SDB_DMS_EOC == rc )
            {
               allInvalid = TRUE ;
               rc = SDB_OK ;
            }
            else
            {
               allInvalid = FALSE ;
               rtnKillContexts( 1, &contextID, cb, rtnCB ) ;
               contextID = -1 ;
            }
         }

         if ( allInvalid )
         {
            break ;
         }
         ossSleep( OSS_ONE_SEC ) ;
         PD_LOG( PDINFO,
                 "Wait for the indexe[%s:%s] on all data nodes to be invalid "
                 "or not exist", collectionName, indexName ) ;
      }

   done:
      if ( contextID != -1 )
      {
         rtnKillContexts( 1, &contextID, cb, rtnCB ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _clsIndexJob::_needRetry( INT32 rc, BOOLEAN &retryLater )
   {
      BOOLEAN needRetry = _rtnIndexJob::_needRetry( rc, retryLater ) ;

      if ( needRetry )
      {
         // if the scanner is interrupted, restart the job later
         // NOTE: the job may be canceled by drop collection space,
         // so need check later
         if ( SDB_DMS_SCANNER_INTERRUPT == rc )
         {
            retryLater = TRUE ;
         }
         if ( retryLater )
         {
            _retryLater = TRUE ;
         }
      }

      return needRetry ;
   }

   INT32 clsStartIndexJob( RTN_JOB_TYPE jobType, UINT32 locationID,
                           clsIdxTask* pTask )
   {
      // _clsMgr::startTaskThread() call this function. Don't execute for too
      // long, or the main thread will be blocked.
      INT32 rc = SDB_OK ;
      rtnIndexJob *job = NULL ;
      dmsTaskStatusPtr statusPtr ;
      dmsTaskStatusMgr *pStatMgr = sdbGetRTNCB()->getTaskStatusMgr() ;

      if ( pStatMgr->findItem( pTask->taskID(), statusPtr ) )
      {
         // Create/Drop index as slave node before upgrading to primary node.
         dmsIdxTaskStatusPtr idxStatPtr =
                     boost::dynamic_pointer_cast<dmsIdxTaskStatus>(statusPtr) ;
         idxStatPtr->setLocationID( locationID ) ;
         rc = clsRestartIndexJob( jobType, idxStatPtr ) ;
      }
      else
      {
         job = SDB_OSS_NEW clsIndexJob( jobType, locationID, pTask ) ;
         PD_CHECK( job != NULL, SDB_OOM, error,
                   PDERROR, "Failed to alloc memory for index job" ) ;

         rc = job->init() ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to init index job, collection[%s] index[%s], rc: %d",
                      pTask->collectionName(), pTask->indexName(), rc ) ;

         rc = rtnGetJobMgr()->startJob( job, RTN_JOB_MUTEX_STOP_CONT ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start job[%s], rc: %d",
                      job->name(), rc ) ;
      }

   done :
      return rc ;
   error :
      if ( job )
      {
         SDB_OSS_DEL job ;
      }
      goto done ;
   }

   INT32 clsStartRollbackIndexJob( RTN_JOB_TYPE jobType,
                                   dmsIdxTaskStatusPtr idxStatPtr,
                                   BOOLEAN forCancel )
   {
      INT32 rc = SDB_OK ;
      rtnIndexJob *job = NULL ;

      job = SDB_OSS_NEW clsIndexJob( jobType, idxStatPtr,
                                     forCancel ? CLS_INDEX_ROLLBACK_CANCEL :
                                                 CLS_INDEX_ROLLBACK ) ;
      PD_CHECK( job != NULL, SDB_OOM, error,
                PDERROR, "Failed to alloc memory for index job" ) ;

      rc = job->init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init index job, "
                   "collection[%llu] index[%s], rollback task[%llu], rc: %d",
                   idxStatPtr->clUniqueID(), idxStatPtr->indexName(),
                   idxStatPtr->taskID(), rc ) ;

      rc = rtnGetJobMgr()->startJob( job, RTN_JOB_MUTEX_STOP_CONT ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start job[%s], rc: %d",
                   job->name(), rc ) ;

   done :
      return rc ;
   error :
      if ( job )
      {
         SDB_OSS_DEL job ;
      }
      goto done ;
   }

   INT32 clsRestartIndexJob( RTN_JOB_TYPE jobType,
                             dmsIdxTaskStatusPtr idxStatPtr )
   {
      INT32 rc = SDB_OK ;
      rtnIndexJob *job = NULL ;

      job = SDB_OSS_NEW clsIndexJob( jobType, idxStatPtr, CLS_INDEX_RESTART ) ;
      PD_CHECK( job != NULL, SDB_OOM, error,
                PDERROR, "Failed to alloc memory for index job" ) ;

      rc = job->init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init index job, "
                   "collection[%llu] index[%s], restart task[%llu], rc: %d",
                   idxStatPtr->clUniqueID(), idxStatPtr->indexName(),
                   idxStatPtr->taskID(), rc ) ;

      idxStatPtr->setPauseReport( TRUE ) ;

      rc = rtnGetJobMgr()->startJob( job, RTN_JOB_MUTEX_STOP_CONT ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start job[%s], rc: %d",
                   job->name(), rc ) ;

   done :
      return rc ;
   error :
      if ( job )
      {
         SDB_OSS_DEL job ;
      }
      goto done ;
   }

   #define CLS_REPORT_TASK_INFO_INTERVAL ( 5 * OSS_ONE_SEC ) /// ms

   #define CLS_TASK_STATUS_EMPTY_COUNT   ( 100 ) // about 8 minutes

   _clsReportTaskInfoJob::_clsReportTaskInfoJob()
   {
      _pClsCB = pmdGetKRCB()->getClsCB() ;
      _pShardCB = _pClsCB->getShardCB() ;
      _pTaskStatMgr = pmdGetKRCB()->getRTNCB()->getTaskStatusMgr() ;
   }

   void _clsReportTaskInfoJob::_onDone()
   {
      // If thread A is about to exit, but job hasn't been erased from
      // rtnJobMgr::_mapJobs. At this time, another thread B is about to start,
      // but conflict with job A. So we should check again after job is erased.
      if ( !PMD_IS_DB_DOWN() && pmdIsPrimary() && !eduCB()->isForced() )
      {
         if ( _pTaskStatMgr->hasTaskToReport() )
         {
            clsStartReportTaskInfoJob() ;
         }
      }
   }

   RTN_JOB_TYPE _clsReportTaskInfoJob::type() const
   {
      return RTN_JOB_TASKINFO_UPDATE ;
   }

   const CHAR* _clsReportTaskInfoJob::name() const
   {
      return "Report-Task-Info-To-Catalog-Job" ;
   }

   BOOLEAN _clsReportTaskInfoJob::muteXOn( const _rtnBaseJob * pOther )
   {
      if ( type() == pOther->type() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSTASKUPDJOB_DOIT , "_clsReportTaskInfoJob::doit" )
   INT32 _clsReportTaskInfoJob::doit()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSTASKUPDJOB_DOIT ) ;

      pmdEDUCB* cb       = eduCB() ;
      pmdEDUMgr* pEduMgr = pmdGetKRCB()->getEDUMgr() ;
      ossEvent* event    = _pClsCB->getTaskEvent() ;
      INT16 cntEmpty     = 0 ;
      BOOLEAN needStartTaskCheck = FALSE ;

      PD_LOG( PDDEBUG, "Start job[%s]", name() ) ;

      while ( !PMD_IS_DB_DOWN() && pmdIsPrimary() && !cb->isForced() )
      {
         /// Before any one is found in the queue, the status of this thread is
         /// wait. Once found, it will be changed to running.
         pEduMgr->waitEDU( cb->getID() ) ;
         event->wait( CLS_REPORT_TASK_INFO_INTERVAL ) ;
         pEduMgr->activateEDU( cb ) ;
         event->reset() ;
         needStartTaskCheck = FALSE ;

         if ( PMD_IS_DB_DOWN() || !pmdIsPrimary() || cb->isForced() )
         {
            break ;
         }

         /// dump all tasks
         ossPoolMap<UINT64, BSONObj> statusMap ;
         rc = _pTaskStatMgr->dumpForReport( statusMap ) ;

         if ( SDB_OK == rc && statusMap.empty() )
         {
            cntEmpty++ ;
            if ( cntEmpty > CLS_TASK_STATUS_EMPTY_COUNT )
            {
               PD_LOG( PDDEBUG, "Task status map is empty for a while, "
                       "exit job[%s]", name() ) ;
               break ;
            }
         }
         else
         {
            cntEmpty = 0 ;
            needStartTaskCheck = TRUE ;
         }

         /// loop every task and update task progress to catalog
         ossPoolMap<UINT64, BSONObj>::iterator it ;
         for ( it = statusMap.begin() ; it != statusMap.end() ; it++ )
         {
            if ( PMD_IS_DB_DOWN() || !pmdIsPrimary() || cb->isForced() )
            {
               break ;
            }
            rc = _reportTaskInfo2Cata( it->first, it->second ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Failed to update task[%llu] info, rc: %d",
                       it->first, rc ) ;
            }
         }

         if ( needStartTaskCheck &&
              !_pTaskStatMgr->hasTaskToReport() &&
              0 == _pClsCB->getTaskMgr()->idxTaskCount() )
         {
            _pClsCB->startAllTaskCheck() ;
         }
      }

      PD_TRACE_EXITRC ( SDB__CLSTASKUPDJOB_DOIT, rc ) ;
      return rc ;
   }

   INT32 _clsReportTaskInfoJob::_reportTaskInfo2Cata( UINT64 cataTaskID,
                                                      const BSONObj &statObj )
   {
      INT32 rc        = SDB_OK ;
      CHAR *buff      = NULL ;
      INT32 buffSize  = 0 ;
      MsgHeader *msg  = NULL ;
      MsgOpReply *res = NULL ;
      CLS_TASK_STATUS cataTaskStat = CLS_TASK_STATUS_END ;
      vector<BSONObj> objList ;
      BSONElement ele ;

      /// build message
      rc = msgBuildQueryMsg( &buff, &buffSize,
                             CMD_ADMIN_PREFIX CMD_NAME_REPORT_TASK_PROGRESS,
                             0, 0, 0, -1, &statObj ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build message, rc: %d",
                   rc ) ;

      /// update task progress to catalog
      rc = _pShardCB->syncSend( (MsgHeader*)buff, CATALOG_GROUPID,
                                TRUE, &msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send message, rc: %d", rc ) ;

      PD_LOG ( PDDEBUG, "Report task[%llu] progress to catalog", cataTaskID ) ;

      res = ( MsgOpReply* )msg ;

      /// if catalog reply with error code
      if ( SDB_CLS_NOT_PRIMARY == res->flags )
      {
         PD_LOG( PDWARNING, "Catalog is not primary, task[%llu]", cataTaskID ) ;

         if ( SDB_OK != _pShardCB->updatePrimaryByReply( msg ) )
         {
            _pClsCB->updateCatGroup() ;
         }
         goto done ;
      }
      else if ( SDB_DMS_EOC           == res->flags ||
                SDB_CAT_TASK_NOTFOUND == res->flags )
      {
         PD_LOG( PDWARNING, "Task[%llu] doesn't exist on catalog", cataTaskID ) ;

         // drop cs/cl will remove catalog's task, just set finish
         dmsTaskStatusPtr statusPtr ;
         if ( _pTaskStatMgr->findItem( cataTaskID, statusPtr ) )
         {
            statusPtr->setHasTaskInCatalog( FALSE ) ;
         }
         goto done ;
      }
      else if ( SDB_CAT_TASK_STATUS_ERROR == res->flags )
      {
         PD_LOG( PDWARNING, "Failed to update task info[%llu], rc: %d",
                 cataTaskID, res->flags ) ;
         ossSleep( OSS_ONE_SEC ) ;
         rc = _pClsCB->restartTaskThread( cataTaskID ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to restart thread for task[%llu], rc: %d",
                      cataTaskID, rc ) ;
         goto done ;
      }
      else if ( res->flags )
      {
         PD_LOG( PDERROR, "Failed to update task info[%llu], rc: %d",
                 cataTaskID, res->flags ) ;
         goto error ;
      }

      /// if catalog reply without error, extract status from reply
      rc = msgExtractReply( (CHAR *)msg, NULL, NULL, NULL, NULL, objList ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to extract reply, rc: %d",
                   rc ) ;

      ele = objList[0].getField ( FIELD_NAME_STATUS ) ;
      PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                "Invalid field[%s] type[%d] from reply",
                FIELD_NAME_STATUS, ele.type() ) ;

      cataTaskStat = ( CLS_TASK_STATUS )ele.numberInt() ;
      PD_LOG( PDINFO,
              "Get task[%llu] status[%s] from catalog reply",
              cataTaskID, clsTaskStatusStr( cataTaskStat ) ) ;

      /// process different task status
      if ( CLS_TASK_STATUS_FINISH == cataTaskStat )
      {
         dmsTaskStatusPtr statusPtr ;
         if ( _pTaskStatMgr->findItem( cataTaskID, statusPtr ) )
         {
            statusPtr->setHasTaskInCatalog( FALSE ) ;
         }
      }
      else if ( CLS_TASK_STATUS_ROLLBACK == cataTaskStat ||
                CLS_TASK_STATUS_CANCELED == cataTaskStat )
      {
         INT32 resultCode = SDB_OK ;
         rc = rtnGetIntElement( statObj, FIELD_NAME_RESULTCODE, resultCode ) ;
         if ( SDB_OK == rc )
         {
            if ( resultCode != SDB_IXM_EXIST &&
                 resultCode != SDB_IXM_REDEF &&
                 resultCode != SDB_IXM_EXIST_COVERD_ONE )
            {
               // If the index already exists, we don't rollback it
               BOOLEAN isCancel = CLS_TASK_STATUS_CANCELED == cataTaskStat ;
               rc = _pClsCB->startRollbackTaskThread( cataTaskID, isCancel ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to start rollback thread, rc: %d",
                            rc ) ;
            }
         }
         else
         {
            PD_LOG( PDWARNING, "Failed to get field[%s] from task status",
                    FIELD_NAME_RESULTCODE ) ;
         }
      }

   done:
      if ( buff )
      {
         SDB_OSS_FREE( buff ) ;
         buff = NULL ;
      }
      if ( res )
      {
         SDB_OSS_FREE( ( CHAR* )res ) ;
         res = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 clsStartReportTaskInfoJob( EDUID *pEDUID )
   {
      INT32 rc = SDB_OK ;
      _clsReportTaskInfoJob *pJob = NULL ;

      pJob = SDB_OSS_NEW _clsReportTaskInfoJob() ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate failed" ) ;
         goto error ;
      }

      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_RET ) ;

   done:
      return rc ;
   error:
      goto done ;
   }
}

