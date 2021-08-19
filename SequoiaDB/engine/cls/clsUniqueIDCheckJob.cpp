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

   Source File Name = clsUniqueIDCheckJob.cpp

   Descriptive Name = CS/CL UniqueID Checking Job

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who     Description
   ====== =========== ======= ==============================================
          06/08/2018  Ting YU Initial Draft

   Last Changed =

*******************************************************************************/
#include "clsUniqueIDCheckJob.hpp"
#include "pmd.hpp"
#include "dmsStorageUnit.hpp"
#include "rtnLocalTask.hpp"
#include "rtn.hpp"
#include "clsTrace.hpp"

namespace engine
{

   #define CLS_UNIQUEID_CHECK_INTERVAL ( OSS_ONE_SEC * 3 )

   /*
    *  _clsUniqueIDCheckJob implement
    */
   _clsUniqueIDCheckJob::_clsUniqueIDCheckJob( BOOLEAN needPrimary )
   {
      _needPrimary = needPrimary ;
   }

   _clsUniqueIDCheckJob::~_clsUniqueIDCheckJob()
   {
   }

   BOOLEAN _clsUniqueIDCheckJob::muteXOn( const _rtnBaseJob *pOther )
   {
      if ( type() == pOther->type() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSUIDCHKJOB_DOIT, "_clsUniqueIDCheckJob::doit" )
   INT32 _clsUniqueIDCheckJob::doit()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSUIDCHKJOB_DOIT ) ;

      pmdEDUCB *cb = eduCB() ;
      pmdKRCB* pKrcb = pmdGetKRCB() ;
      SDB_DMSCB *pDmsCB = pKrcb->getDMSCB() ;
      SDB_DPSCB *pDpsCB = _needPrimary ? pKrcb->getDPSCB() : NULL ;
      shardCB* pShdMgr = sdbGetShardCB() ;
      clsDCMgr* pDcMgr = pShdMgr->getDCMgr() ;
      UINT64 loopCnt = 0 ;
      BOOLEAN isCataReady = FALSE ;

      PD_LOG( PDEVENT,
              "Start job[%s]: check cs/cl unique id by cs/cl name", name() ) ;

      while ( !PMD_IS_DB_DOWN() &&
              !cb->isForced() &&
              pDmsCB->nullCSUniqueIDCnt() > 0 )
      {
         if ( _needPrimary && !pmdIsPrimary() )
         {
            break ;
         }

         /*
          * Before any one is found in the queue, the status of this thread is
          * wait. Once found, it will be changed to running.
          */
         if ( loopCnt++ != 0 )
         {
            pmdEDUEvent event ;
            cb->waitEvent( event, CLS_UNIQUEID_CHECK_INTERVAL, TRUE ) ;
         }

         // 1. check if the cs/cl unique id on catalog have been generated.
         if ( !isCataReady )
         {
            rc = pShdMgr->updateDCBaseInfo() ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Job[%s]: "
                       "Update data center base info failed, rc: %d",
                       name(), rc ) ;
               continue ;
            }

            clsDCBaseInfo* pDcInfo = pDcMgr->getDCBaseInfo() ;
            if ( !pDcInfo->hasCSUniqueHWM() )
            {
               continue ;
            }
         }
         isCataReady = TRUE ;

         // 2. loop each cs
         MON_CS_LIST csList ;
         MON_CS_LIST::const_iterator iterCS ;

         pDmsCB->dumpInfo( csList, FALSE ) ;

         for ( iterCS = csList.begin() ; iterCS != csList.end(); ++iterCS )
         {
            const _monCollectionSpace &cs = *iterCS ;
            utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
            BSONObj clInfoObj ;

            PD_LOG( PDDEBUG,
                    "Job[%s]: Check collection space[%s]",
                    name(), cs._name ) ;

            if ( PMD_IS_DB_DOWN() ||
                 cb->isForced() ||
                 ( _needPrimary && !pmdIsPrimary() ) )
            {
               break ;
            }

            // we only need to operate cs which unique id = 0
            if ( cs._csUniqueID != UTIL_UNIQUEID_NULL )
            {
               continue ;
            }

            // update catalog info
            rc = pShdMgr->rGetCSInfo( cs._name, csUniqueID,
                                      NULL, NULL, NULL, &clInfoObj ) ;
            if ( rc != SDB_OK )
            {
               PD_LOG( PDWARNING,
                       "Job[%s]: Update cs[%s] catalog info, rc: %d. "
                       "CS doesn't exist in catalog",
                       name(), cs._name, rc ) ;
               if ( rc != SDB_DMS_CS_NOTEXIST )
               {
                  continue ;
               }
            }

            if ( SDB_DMS_CS_NOTEXIST == rc )
            {
               rc = rtnChangeUniqueID( cs._name, UTIL_CSUNIQUEID_LOCAL,
                                       BSONObj(),
                                       cb, pDmsCB, pDpsCB ) ;
            }
            else
            {
               rc = rtnChangeUniqueID( cs._name, csUniqueID,
                                       clInfoObj,
                                       cb, pDmsCB, pDpsCB ) ;
            }

            if ( rc )
            {
               PD_LOG( PDWARNING,
                       "Job[%s]: Change cs[%s] unique id failed, rc: %d",
                       name(), cs._name, rc ) ;
               continue ;
            }

         }// end for

      }// end while

      PD_LOG( PDEVENT, "Stop job[%s]", name() ) ;

      PD_TRACE_EXITRC( SDB__CLSUIDCHKJOB_DOIT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_STARTUIDCHKJOB, "startUniqueIDCheckJob" )
   INT32 startUniqueIDCheckJob ( BOOLEAN needPrimary, EDUID* pEDUID )
   {
      PD_TRACE_ENTRY( SDB_STARTUIDCHKJOB ) ;

      INT32 rc = SDB_OK ;
      clsUniqueIDCheckJob *pJob = NULL ;

      pJob = SDB_OSS_NEW clsUniqueIDCheckJob( needPrimary ) ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate failed" ) ;
         goto error ;
      }
      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_STOP_CONT, pEDUID ) ;

   done:
      PD_TRACE_EXITRC( SDB_STARTUIDCHKJOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   #define CLS_RENAME_RETRY_TIME             ( 3 * OSS_ONE_SEC )
   #define CLS_RENAME_PRIMARY_TIMEOUT        ( 5 * OSS_ONE_SEC )
   #define CLS_NAME_CHECK_PRIMARY_INTERVAL   ( 100 )
   /*
      _clsRenameCheckJob implement
   */
   _clsRenameCheckJob::_clsRenameCheckJob( const rtnLocalTaskPtr &taskPtr,
                                           UINT64 opID )
   {
      _tick = pmdGetDBTick() ;
      shardCB *shardCB = pmdGetKRCB()->getClsCB()->getShardCB() ;
      _pFreezeWindow = shardCB->getFreezingWindow() ;
      _taskPtr = taskPtr ;
      _opID = opID ;
   }

   _clsRenameCheckJob::~_clsRenameCheckJob()
   {
      _release() ;
   }

   void _clsRenameCheckJob::_release()
   {
      if ( 0 != _opID )
      {
         rtnLTRename *pRename = (rtnLTRename*)_taskPtr.get() ;

         if ( RTN_LOCAL_TASK_RENAMECS == _taskPtr->getTaskType() )
         {
            _pFreezeWindow->unregisterCS( pRename->getFrom(), _opID ) ;
            PD_LOG( PDEVENT, "End to block all write operations of "
                    "collectionspace(%s), ID: %llu",
                    pRename->getFrom(), _opID ) ;
         }
         else
         {
            _pFreezeWindow->unregisterCL( pRename->getFrom(), _opID ) ;
            PD_LOG( PDEVENT, "End to block all write operations of "
                    "collection(%s), ID: %llu",
                    pRename->getFrom(), _opID ) ;
         }

         _opID = 0 ;
      }
   }

   const CHAR* _clsRenameCheckJob::name() const
   {
      if ( _taskPtr.get() )
      {
         return rtnLocalTaskType2Str( _taskPtr->getTaskType() ) ;
      }
      return "Unknown" ;
   }

   INT32 _clsRenameCheckJob::init()
   {
      INT32 rc = SDB_OK ;

      if ( 0 == _opID )
      {
         rtnLTRename *pRename = (rtnLTRename*)_taskPtr.get() ;

         if ( RTN_LOCAL_TASK_RENAMECS == _taskPtr->getTaskType() )
         {
            rc = _pFreezeWindow->registerCS( pRename->getFrom(), _opID ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Block all write operations of "
                       "collectionspace[%s] failed, rc: %d",
                       pRename->getFrom(), rc ) ;
            }
            else
            {
               PD_LOG( PDEVENT, "Begin to block all write operations "
                       "of collectionspace[%s], ID: %llu", pRename->getFrom(),
                       _opID ) ;
            }
         }
         else
         {
            rc = _pFreezeWindow->registerCL( pRename->getFrom(), _opID ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Block all write operations of "
                       "collection[%s] failed, rc: %d",
                       pRename->getFrom(), rc ) ;
            }
            else
            {
               PD_LOG( PDEVENT, "Begin to block all write operations "
                       "of collection[%s], ID: %llu", pRename->getFrom(),
                       _opID ) ;
            }
         }
      }

      return rc ;
   }

   INT32 _clsRenameCheckJob::doit( IExecutor *pExe,
                                   UTIL_LJOB_DO_RESULT &result,
                                   UINT64 &sleepTime )
   {
      INT32 rc = SDB_OK ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      shardCB *pShdMgr = sdbGetShardCB() ;
      clsCB *pClsCB = sdbGetClsCB() ;
      _dpsLogWrapper *dpsCB = pmdGetKRCB()->getDPSCB() ;
      _rtnLocalTaskMgr *pLTMgr = pmdGetKRCB()->getRTNCB()->getLTMgr() ;

      rtnLTRename *pRename = (rtnLTRename*)_taskPtr.get() ;
      BOOLEAN needRemove = FALSE ;

      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      _dmsStorageUnit *su = NULL ;

      /*
         When not primary, finish the light job
      */
      if ( !pClsCB->isPrimary() )
      {
         if ( pmdGetTickSpanTime( _tick ) < CLS_RENAME_PRIMARY_TIMEOUT )
         {
            result = UTIL_LJOB_DO_CONT ;
            sleepTime = CLS_NAME_CHECK_PRIMARY_INTERVAL ;
            goto done ;
         }
         else
         {
            goto finish ;
         }
      }
      else if ( PMD_IS_DB_DOWN() )
      {
         goto finish ;
      }

      needRemove = TRUE ;

      if ( RTN_LOCAL_TASK_RENAMECS == _taskPtr->getTaskType() )
      {
         utilCSUniqueID remoteCSUID = UTIL_UNIQUEID_NULL ;
         utilCSUniqueID localCSUID = UTIL_UNIQUEID_NULL ;

         /// Get to cs info
         rc = pShdMgr->rGetCSInfo( pRename->getTo(), remoteCSUID ) ;
         if ( SDB_DMS_CS_NOTEXIST == rc )
         {
            /// The dest collectionspace is not exist, finish
            goto finish ;
         }
         else if ( rc )
         {
            PD_LOG( PDINFO, "Get collectionspace(%s) information failed, "
                    "rc: %d", pRename->getTo(), rc ) ;
            result = UTIL_LJOB_DO_CONT ;
            sleepTime = CLS_RENAME_RETRY_TIME ;
            goto error ;
         }

         /// test from cs
         rc = dmsCB->nameToSUAndLock( pRename->getFrom(), suID, &su ) ;
         if ( SDB_DMS_CS_NOTEXIST == rc )
         {
            /// The source colletionspace is not exist, finish
            goto finish ;
         }
         else if ( rc )
         {
            PD_LOG( PDWARNING, "Lock collectionspace(%s) failed, rc: %d",
                    pRename->getFrom(), rc ) ;
            result = UTIL_LJOB_DO_CONT ;
            sleepTime = CLS_RENAME_RETRY_TIME ;
            goto error ;
         }

         localCSUID = su->CSUniqueID() ;

         dmsCB->suUnlock( suID ) ;
         suID = DMS_INVALID_SUID ;

         /// check suid
         if ( remoteCSUID != localCSUID )
         {
            goto finish ;
         }

         /// rename cs
         rc = rtnRenameCollectionSpaceCommand( pRename->getFrom(),
                                               pRename->getTo(),
                                               (pmdEDUCB*)pExe,
                                               dmsCB,
                                               dpsCB,
                                               FALSE ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Rename collectionspace(%s) to (%s) failed, "
                    "rc: %d", pRename->getFrom(), pRename->getTo(), rc ) ;
            result = UTIL_LJOB_DO_CONT ;
            sleepTime = CLS_RENAME_RETRY_TIME ;
            goto error ;
         }

         goto finish ;
      }
      else
      {
         utilCLUniqueID remoteCLUID = UTIL_UNIQUEID_NULL ;
         utilCLUniqueID localCLUID = UTIL_UNIQUEID_NULL ;
         clsCatalogSet *pSet = NULL ;
         UINT32 groupCount = 0 ;
         const CHAR *pShortName = NULL ;
         const CHAR *pNewShortName = NULL ;
         UINT16 mbID = DMS_INVALID_MBID ;
         CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;

         pNewShortName = ossStrchr( pRename->getTo(), '.' ) + 1 ;

         /// clear local catalog info
         pShdMgr->getCataAgent()->lock_w() ;
         pShdMgr->getCataAgent()->clear( pRename->getTo() ) ;
         pShdMgr->getCataAgent()->release_w() ;

         /// Get to cl info
         rc = pShdMgr->getAndLockCataSet( pRename->getTo(), &pSet ) ;
         if ( SDB_DMS_NOTEXIST == rc )
         {
            /// The dest collection is not exist, finish
            goto finish ;
         }
         else if ( rc )
         {
            PD_LOG( PDINFO, "Update collection(%s)'s catalog information "
                    "failed, rc: %d", pRename->getTo(), rc ) ;
            result = UTIL_LJOB_DO_CONT ;
            sleepTime = CLS_RENAME_RETRY_TIME ;
            goto error ;
         }

         remoteCLUID = pSet->clUniqueID() ;
         groupCount = pSet->groupCount() ;
         pShdMgr->unlockCataSet( pSet ) ;

         if ( 0 == groupCount )
         {
            /// The collection is not on the group
            pShdMgr->getCataAgent()->lock_w() ;
            pShdMgr->getCataAgent()->clear( pRename->getTo() ) ;
            pShdMgr->getCataAgent()->release_w() ;

            goto finish ;
         }

         /// get local cl unique id
         rc = rtnResolveCollectionNameAndLock( pRename->getFrom(), dmsCB,
                                               &su, &pShortName, suID ) ;
         if ( SDB_DMS_CS_NOTEXIST == rc )
         {
            /// The source collectionspace not exist, finish
            goto finish ;
         }
         else if ( rc )
         {
            PD_LOG( PDWARNING, "Lock collectionspace(%s) failed, rc: %d",
                    pRename->getFrom(), rc ) ;
            result = UTIL_LJOB_DO_CONT ;
            sleepTime = CLS_RENAME_RETRY_TIME ;
            goto error ;
         }

         /// copy cs name
         ossStrncpy( csName, pRename->getFrom(),
                     pShortName - pRename->getFrom() - 1 ) ;

         rc = su->data()->findCollection( pShortName, mbID, &localCLUID ) ;
         if ( SDB_DMS_NOTEXIST == rc )
         {
            /// The source collection not exist, finish
            goto finish ;
         }
         else if ( rc )
         {
            PD_LOG( PDWARNING, "Get collection(%s)'s unique id failed, rc: %d",
                    pRename->getFrom(), rc ) ;
            result = UTIL_LJOB_DO_CONT ;
            sleepTime = CLS_RENAME_RETRY_TIME ;
         }

         dmsCB->suUnlock( suID ) ;
         suID = DMS_INVALID_SUID ;

         /// check unique id
         if ( remoteCLUID != localCLUID )
         {
            goto finish ;
         }

         /// rename cl
         rc = rtnRenameCollectionCommand( csName,
                                          pShortName,
                                          pNewShortName,
                                          (pmdEDUCB*)pExe,
                                          dmsCB,
                                          dpsCB,
                                          FALSE ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Rename collection(%s) to (%s) failed, "
                    "rc: %d", pRename->getFrom(), pRename->getTo(), rc ) ;
            result = UTIL_LJOB_DO_CONT ;
            sleepTime = CLS_RENAME_RETRY_TIME ;
            goto error ;
         }

         goto finish ;
      }

   finish:
      rc = SDB_OK ;
      result = UTIL_LJOB_DO_FINISH ;
      _release() ;
      if ( needRemove && pClsCB->isPrimary() )
      {
         pLTMgr->removeTask( _taskPtr,(pmdEDUCB*)pExe, dpsCB ) ;
      }
   done:
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 clsStartRenameCheckJob( const rtnLocalTaskPtr &taskPtr, UINT64 opID )
   {
      INT32 rc = SDB_OK ;
      _clsRenameCheckJob *pJob = NULL ;

      if ( !taskPtr.get() )
      {
         goto done ;
      }

      pJob = SDB_OSS_NEW _clsRenameCheckJob( taskPtr, opID ) ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Alloc rename check job failed" ) ;
         goto error ;
      }

      rc = pJob->init() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init rename check job(%s) failed, rc: %d",
                 taskPtr->toPrintString().c_str(), rc ) ;
         goto error ;
      }

      rc = pJob->submit() ;
      pJob = NULL ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Submit rename check job(%s) failed, rc: %d",
                 taskPtr->toPrintString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( pJob )
      {
         SDB_OSS_DEL pJob ;
      }
      goto done ;
   }

   INT32 clsStartRenameCheckJobs()
   {
      INT32 rc = SDB_OK ;
      rtnLocalTaskMgr *pLTMgr = pmdGetKRCB()->getRTNCB()->getLTMgr() ;
      _rtnLocalTaskMgr::MAP_TASK mapTask ;
      _rtnLocalTaskMgr::MAP_TASK_IT it ;
      UINT32 succeed = 0 ;
      UINT32 failed = 0 ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      rc = pLTMgr->reload( cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Reload local tasks failed, rc: %d", rc ) ;
         /// ignore
      }

      rc = pLTMgr->dumpTask( mapTask) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Dump local task failed, rc: %d", rc ) ;
         /// ignore
      }

      it = mapTask.begin() ;
      while( it != mapTask.end() )
      {
         const rtnLocalTaskPtr &taskPtr = it->second ;

         if ( RTN_LOCAL_TASK_RENAMECS == taskPtr->getTaskType() ||
              RTN_LOCAL_TASK_RENAMECL == taskPtr->getTaskType() )
         {
            rc = clsStartRenameCheckJob( it->second, 0 ) ;
            if ( rc )
            {
               ++failed ;
            }
            else
            {
               ++succeed ;
            }
         }
         else
         {
            PD_LOG( PDWARNING, "Unknown local task(%s)",
                    taskPtr->toPrintString().c_str() ) ;
         }
         ++it ;
      }

      if ( succeed + failed > 0 )
      {
         PD_LOG( PDEVENT, "Start name check jos, succed: %u, failed: %u",
                 succeed, failed ) ;
      }

      return rc ;
   }

}
