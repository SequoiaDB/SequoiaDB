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
#include "clsRecycleBinJob.hpp"

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
      clsCB *clsCB = pmdGetKRCB()->getClsCB() ;
      shardCB *shardCB = clsCB->getShardCB() ;
      _pFreezeWindow = shardCB->getFreezingWindow() ;
      _recycleBinMgr = clsCB->getRecycleBinMgr() ;
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

         switch ( _taskPtr->getTaskType() )
         {
            case RTN_LOCAL_TASK_RENAMECS :
            {
               _pFreezeWindow->unregisterCS( pRename->getFrom(), _opID ) ;
               PD_LOG( PDEVENT, "End to block all write operations of "
                       "collectionspace(%s), ID: %llu",
                       pRename->getFrom(), _opID ) ;
               break ;
            }
            case RTN_LOCAL_TASK_RENAMECL :
            {
               _pFreezeWindow->unregisterCL( pRename->getFrom(), _opID ) ;
               PD_LOG( PDEVENT, "End to block all write operations of "
                       "collection(%s), ID: %llu",
                       pRename->getFrom(), _opID ) ;
               break ;
            }
            case RTN_LOCAL_TASK_RECYCLECS :
            case RTN_LOCAL_TASK_RETURNCS :
            {
               _pFreezeWindow->unregisterCS( pRename->getFrom(), _opID ) ;
               PD_LOG( PDEVENT, "End to block all write operations of "
                       "collectionspace(%s), ID: %llu",
                       pRename->getFrom(), _opID ) ;
               _pFreezeWindow->unregisterCS( pRename->getTo(), _opID ) ;
               PD_LOG( PDEVENT, "End to block all write operations of "
                       "collectionspace(%s), ID: %llu",
                       pRename->getTo(), _opID ) ;
               break ;
            }
            case RTN_LOCAL_TASK_RECYCLECL :
            case RTN_LOCAL_TASK_RETURNCL :
            {
               _pFreezeWindow->unregisterCL( pRename->getFrom(), _opID ) ;
               PD_LOG( PDEVENT, "End to block all write operations of "
                       "collection(%s), ID: %llu",
                       pRename->getFrom(), _opID ) ;
               _pFreezeWindow->unregisterCL( pRename->getTo(), _opID ) ;
               PD_LOG( PDEVENT, "End to block all write operations of "
                       "collection(%s), ID: %llu",
                       pRename->getTo(), _opID ) ;
               break ;
            }
            default :
            {
               SDB_ASSERT( FALSE, "invalid type of local task" ) ;
               PD_LOG( PDWARNING, "Unknown local task(%s)",
                       _taskPtr->toPrintString().c_str() ) ;
               break ;
            }
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

      // No need to set white list
      // - if opID is valid, the rename context already set white list
      // - if opID is invalid, it is triggered by primary switching, so there
      //   should be no transactions at the same time
      if ( 0 == _opID )
      {
         rtnLTRename *pRename = (rtnLTRename*)_taskPtr.get() ;

         switch ( _taskPtr->getTaskType() )
         {
            case RTN_LOCAL_TASK_RENAMECS :
            {
               rc = _pFreezeWindow->registerCS( pRename->getFrom(), _opID ) ;
               PD_RC_CHECK( rc, PDERROR, "Block all write operations of "
                            "collectionspace[%s] failed, rc: %d",
                            pRename->getFrom(), rc ) ;
               PD_LOG( PDEVENT, "Begin to block all write operations "
                       "of collectionspace[%s], ID: %llu",
                       pRename->getFrom(), _opID ) ;
               break ;
            }
            case RTN_LOCAL_TASK_RENAMECL :
            {
               rc = _pFreezeWindow->registerCL( pRename->getFrom(), _opID ) ;
               PD_RC_CHECK( rc, PDERROR, "Block all write operations of "
                            "collection[%s] failed, rc: %d",
                            pRename->getFrom(), rc ) ;
               PD_LOG( PDEVENT, "Begin to block all write operations "
                       "of collection[%s], ID: %llu", pRename->getFrom(),
                       _opID ) ;
               break ;
            }
            case RTN_LOCAL_TASK_RECYCLECS :
            case RTN_LOCAL_TASK_RETURNCS :
            {
               rc = _pFreezeWindow->registerCS( pRename->getFrom(), _opID ) ;
               PD_RC_CHECK( rc, PDERROR, "Block all write operations of "
                            "collectionspace[%s] failed, rc: %d",
                            pRename->getFrom(), rc ) ;
               PD_LOG( PDEVENT, "Begin to block all write operations "
                       "of collectionspace[%s], ID: %llu",
                       pRename->getFrom(), _opID ) ;
               rc = _pFreezeWindow->registerCS( pRename->getTo(), _opID ) ;
               PD_RC_CHECK( rc, PDERROR, "Block all write operations of "
                            "collectionspace[%s] failed, rc: %d",
                            pRename->getTo(), rc ) ;
               PD_LOG( PDEVENT, "Begin to block all write operations "
                       "of collectionspace[%s], ID: %llu",
                       pRename->getTo(), _opID ) ;
               break ;
            }
            case RTN_LOCAL_TASK_RECYCLECL :
            case RTN_LOCAL_TASK_RETURNCL :
            {
               rc = _pFreezeWindow->registerCL( pRename->getFrom(), _opID ) ;
               PD_RC_CHECK( rc, PDERROR, "Block all write operations of "
                            "collection[%s] failed, rc: %d",
                            pRename->getFrom(), rc ) ;
               PD_LOG( PDEVENT, "Begin to block all write operations "
                       "of collection[%s], ID: %llu",
                       pRename->getFrom(), _opID ) ;
               rc = _pFreezeWindow->registerCL( pRename->getTo(), _opID ) ;
               PD_RC_CHECK( rc, PDERROR, "Block all write operations of "
                            "collection[%s] failed, rc: %d",
                            pRename->getTo(), rc ) ;
               PD_LOG( PDEVENT, "Begin to block all write operations "
                       "of collection[%s], ID: %llu",
                       pRename->getTo(), _opID ) ;
               break ;
            }
            default :
            {
               SDB_ASSERT( FALSE, "invalid type of local task" ) ;
               PD_LOG( PDWARNING, "Unknown local task(%s)",
                       _taskPtr->toPrintString().c_str() ) ;
               rc = SDB_SYS ;
               break ;
            }
         }
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsRenameCheckJob::doit( IExecutor *pExe,
                                   UTIL_LJOB_DO_RESULT &result,
                                   UINT64 &sleepTime )
   {
      INT32 rc = SDB_OK ;

      clsCB *pClsCB = sdbGetClsCB() ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      _rtnLocalTaskMgr *pLTMgr = pmdGetKRCB()->getRTNCB()->getLTMgr() ;

      BOOLEAN needRemove = FALSE ;

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

      switch ( _taskPtr->getTaskType() )
      {
         case RTN_LOCAL_TASK_RENAMECS :
         {
            rc = _doRenameCS( pExe ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDWARNING, "Failed to finish rename collection space "
                       "task, rc: %d", rc ) ;
               result = UTIL_LJOB_DO_CONT ;
               sleepTime = CLS_RENAME_RETRY_TIME ;
               goto error ;
            }
            break ;
         }
         case RTN_LOCAL_TASK_RENAMECL :
         {
            rc = _doRenameCL( pExe ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDWARNING, "Failed to finish rename collection task, "
                       "rc: %d", rc ) ;
               result = UTIL_LJOB_DO_CONT ;
               sleepTime = CLS_RENAME_RETRY_TIME ;
               goto error ;
            }
            break ;
         }
         case RTN_LOCAL_TASK_RECYCLECS :
         {
            rc = _doRenameRecycleCS( pExe ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDWARNING, "Failed to finish recycle collection space "
                       "task, rc: %d", rc ) ;
               result = UTIL_LJOB_DO_CONT ;
               sleepTime = CLS_RENAME_RETRY_TIME ;
               goto error ;
            }
            break ;
         }
         case RTN_LOCAL_TASK_RECYCLECL :
         {
            rc = _doRenameRecycleCL( pExe ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDWARNING, "Failed to finish recycle collection task, "
                       "rc: %d", rc ) ;
               result = UTIL_LJOB_DO_CONT ;
               sleepTime = CLS_RENAME_RETRY_TIME ;
               goto error ;
            }
            break ;
         }
         case RTN_LOCAL_TASK_RETURNCS :
         {
            rc = _doRenameReturnCS( pExe ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDWARNING, "Failed to finish return collection space "
                       "task, rc: %d", rc ) ;
               result = UTIL_LJOB_DO_CONT ;
               sleepTime = CLS_RENAME_RETRY_TIME ;
               goto error ;
            }
            break ;
         }
         case RTN_LOCAL_TASK_RETURNCL :
         {
            rc = _doRenameReturnCL( pExe ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDWARNING, "Failed to finish return collection task, "
                       "rc: %d", rc ) ;
               result = UTIL_LJOB_DO_CONT ;
               sleepTime = CLS_RENAME_RETRY_TIME ;
               goto error ;
            }
            break ;
         }
         default :
         {
            SDB_ASSERT( FALSE, "invalid task type" ) ;
            PD_LOG( PDWARNING, "Unknown local task(%s)",
                    _taskPtr->toPrintString().c_str() ) ;
            break ;
         }
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
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsRenameCheckJob::_doRenameCS( IExecutor *pExe )
   {
      INT32 rc = SDB_OK ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      shardCB *pShdMgr = sdbGetShardCB() ;
      rtnLTRenameCS *pRename = (rtnLTRenameCS *)( _taskPtr.get() ) ;
      pmdEDUCB *cb = (pmdEDUCB *)pExe ;

      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageUnit *su = NULL ;

      utilCSUniqueID remoteCSUID = UTIL_UNIQUEID_NULL ;
      utilCSUniqueID localCSUID = UTIL_UNIQUEID_NULL ;

      /// Get to cs info
      rc = pShdMgr->rGetCSInfo( pRename->getTo(), remoteCSUID ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         /// The dest collectionspace is not exist, finish
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDINFO, "Get collectionspace(%s) information failed, "
                   "rc: %d", pRename->getTo(), rc ) ;

      /// test from cs
      rc = dmsCB->nameToSUAndLock( pRename->getFrom(), suID, &su ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         /// The source colletionspace is not exist, finish
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc,PDWARNING, "Lock collectionspace(%s) failed, rc: %d",
                   pRename->getFrom(), rc ) ;

      localCSUID = su->CSUniqueID() ;

      dmsCB->suUnlock( suID ) ;
      suID = DMS_INVALID_SUID ;

      /// check suid
      if ( remoteCSUID != localCSUID )
      {
         goto done ;
      }

      /// rename cs
      rc = rtnRenameCollectionSpaceCommand( pRename->getFrom(),
                                            pRename->getTo(),
                                            cb,
                                            dmsCB,
                                            dpsCB,
                                            FALSE ) ;
      PD_RC_CHECK( rc, PDWARNING, "Rename collectionspace(%s) to (%s) failed, "
                   "rc: %d", pRename->getFrom(), pRename->getTo(), rc ) ;

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsRenameCheckJob::_doRenameCL( IExecutor *pExe )
   {
      INT32 rc = SDB_OK ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      shardCB *pShdMgr = sdbGetShardCB() ;
      rtnLTRenameCL *pRename = (rtnLTRenameCL *)( _taskPtr.get() ) ;

      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageUnit *su = NULL ;

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
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDINFO, "Update collection(%s)'s catalog information "
                   "failed, rc: %d", pRename->getTo(), rc ) ;

      remoteCLUID = pSet->clUniqueID() ;
      groupCount = pSet->groupCount() ;
      pShdMgr->unlockCataSet( pSet ) ;

      if ( 0 == groupCount )
      {
         /// The collection is not on the group
         pShdMgr->getCataAgent()->lock_w() ;
         pShdMgr->getCataAgent()->clear( pRename->getTo() ) ;
         pShdMgr->getCataAgent()->release_w() ;

         goto done ;
      }

      /// get local cl unique id
      rc = rtnResolveCollectionNameAndLock( pRename->getFrom(), dmsCB,
                                            &su, &pShortName, suID ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         /// The source collectionspace not exist, finish
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Lock collectionspace(%s) failed, rc: %d",
                   pRename->getFrom(), rc ) ;

      /// copy cs name
      ossStrncpy( csName, pRename->getFrom(),
                  pShortName - pRename->getFrom() - 1 ) ;

      rc = su->data()->findCollection( pShortName, mbID, &localCLUID ) ;
      if ( SDB_DMS_NOTEXIST == rc )
      {
         /// The source collection not exist, finish
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Get collection(%s)'s unique id failed, "
                   "rc: %d", pRename->getFrom(), rc ) ;

      dmsCB->suUnlock( suID ) ;
      suID = DMS_INVALID_SUID ;

      /// check unique id
      if ( remoteCLUID != localCLUID )
      {
         goto done ;
      }

      /// rename cl
      rc = rtnRenameCollectionCommand( csName,
                                       pShortName,
                                       pNewShortName,
                                       (pmdEDUCB*)pExe,
                                       dmsCB,
                                       dpsCB,
                                       FALSE ) ;
      PD_RC_CHECK( rc, PDWARNING, "Rename collection(%s) to (%s) failed, "
                   "rc: %d", pRename->getFrom(), pRename->getTo(), rc ) ;

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsRenameCheckJob::_checkCSWithRecyItem( const utilRecycleItem &recycleItem,
                                                   IExecutor *pExe,
                                                   BOOLEAN &isRemoteItemExist,
                                                   BOOLEAN &isRemoteCSExist,
                                                   BOOLEAN &isLocalItemExist,
                                                   BOOLEAN &isLocalCSExist,
                                                   BOOLEAN &isLocalRecyCSExist )
   {
      INT32 rc = SDB_OK ;

      const CHAR *originName = recycleItem.getOriginName() ;
      const CHAR *recycleName = recycleItem.getRecycleName() ;
      utilCSUniqueID originUID = (utilCSUniqueID)( recycleItem.getOriginID() ) ;

      rc = _checkRemoteItem( recycleItem, pExe, isRemoteItemExist ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to check recycle item [origin %s, "
                   "recycle %s] from CATALOG, rc: %d", originName, recycleName,
                   rc ) ;

      rc = _checkLocalItem( recycleItem, pExe, isLocalItemExist ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to check recycle item [origin %s, "
                   "recycle %s], rc: %d", originName, recycleName, rc ) ;

      rc = _checkRemoteCS( originName, originUID, isRemoteCSExist ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to check collection space "
                   "[%s] on CATALOG, rc: %d", originName, rc ) ;

      rc = _checkLocalCS( originName, originUID, isLocalCSExist ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to check local collection space "
                   "[%s], rc: %d", originName, rc ) ;

      rc = _checkLocalCS( recycleName, UTIL_UNIQUEID_NULL, isLocalRecyCSExist ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to check local collection space "
                   "[%s], rc: %d", recycleName, rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsRenameCheckJob::_checkCLWithRecyItem( const utilRecycleItem &recycleItem,
                                                   IExecutor *pExe,
                                                   BOOLEAN &isRemoteItemExist,
                                                   BOOLEAN &isRemoteCLExist,
                                                   BOOLEAN &isLocalItemExist,
                                                   BOOLEAN &isLocalCLExist,
                                                   BOOLEAN &isLocalRecyCLExist )
   {
      INT32 rc = SDB_OK ;

      const CHAR *originName = recycleItem.getOriginName() ;
      const CHAR *recycleName = recycleItem.getRecycleName() ;
      CHAR szSpace[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;
      CHAR origCLShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = { 0 } ;
      utilCLUniqueID originUID = (utilCLUniqueID)( recycleItem.getOriginID() ) ;

      rc = rtnResolveCollectionName( originName, ossStrlen( originName ),
                                     szSpace, DMS_COLLECTION_SPACE_NAME_SZ,
                                     origCLShortName, DMS_COLLECTION_NAME_SZ ) ;
      SDB_ASSERT( SDB_OK == rc, "old collection name is invalid" ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to resolve collection name [%s], "
                   "rc: %d", originName, rc ) ;

      rc = _checkRemoteItem( recycleItem, pExe, isRemoteItemExist ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to check recycle item [origin %s, "
                   "recycle %s] from CATALOG, rc: %d", originName, recycleName,
                   rc ) ;

      rc = _checkLocalItem( recycleItem, pExe, isLocalItemExist ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to check recycle item [origin %s, "
                   "recycle %s], rc: %d", originName, recycleName, rc ) ;

      rc = _checkRemoteCL( szSpace, origCLShortName, originUID,
                           isRemoteCLExist ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to check remote "
                   "collection [%s.%s], rc: %d", szSpace, origCLShortName,
                   rc ) ;

      rc = _checkLocalCL( szSpace, origCLShortName, originUID,
                          isLocalCLExist ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to check local collection [%s.%s], "
                 "rc: %d", szSpace, origCLShortName, rc ) ;

      rc = _checkLocalCL( szSpace, recycleName, UTIL_UNIQUEID_NULL,
                          isLocalRecyCLExist ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to check local recycled "
                   "collection [%s.%s], rc: %d", szSpace, recycleName,
                   rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsRenameCheckJob::_doRenameRecycleCS( IExecutor *pExe )
   {
      INT32 rc = SDB_OK ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      rtnLTRecycleBase *pRename = (rtnLTRecycleBase *)( _taskPtr.get() ) ;
      pmdEDUCB *cb = (pmdEDUCB *)pExe ;

      const utilRecycleItem &recycleItem = pRename->getRecycleItem() ;
      const CHAR *originName = recycleItem.getOriginName() ;
      const CHAR *recycleName = recycleItem.getRecycleName() ;

      BOOLEAN isRemoteItemExist = FALSE,
              isLocalItemExist = FALSE,
              isRemoteCSExist = FALSE,
              isLocalCSExist = FALSE,
              isLocalRecyCSExist = FALSE ;

      rc = _checkCSWithRecyItem( recycleItem, pExe,
                                 isRemoteItemExist,
                                 isRemoteCSExist,
                                 isLocalItemExist,
                                 isLocalCSExist,
                                 isLocalRecyCSExist ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check collection with recycle item "
                   "[origin: %s, recycle: %s], rc: %d",
                   originName, recycleName, rc ) ;

      if ( isRemoteItemExist )
      {
         // need finish job
         if ( isLocalCSExist && !isLocalRecyCSExist )
         {
            // recycle collection space
            dmsDropCSOptions options( recycleItem, FALSE ) ;
            rc = rtnDropCollectionSpaceCommand( originName, cb, dmsCB, dpsCB,
                                                TRUE, FALSE, &options ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to recycle drop collection space "
                         "[%s] to [%s], rc: %d", originName, recycleName, rc ) ;
         }
         if ( !isLocalItemExist )
         {
            // save item
            rc = _recycleBinMgr->saveItem( recycleItem, cb ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to save recycle item "
                         "[origin %s, recycle %s], rc: %d", originName,
                         recycleName, rc ) ;
         }
      }
      else
      {
         PD_LOG( PDWARNING, "Unknown status of recycle collection space "
                 "item [origin:%s, recycle %s]", originName, recycleName ) ;

         if ( isLocalItemExist || isLocalRecyCSExist )
         {
            // drop item and recycled collection space
            BOOLEAN isDropped = FALSE ;
            rc = _recycleBinMgr->dropItemWithCheck( recycleItem, cb, FALSE,
                                                    isDropped ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to drop recycle item "
                         "[origin %s, recycle %s], rc: %d", originName,
                         recycleName, rc ) ;
         }
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsRenameCheckJob::_doRenameReturnCS( IExecutor *pExe )
   {
      INT32 rc = SDB_OK ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      rtnLTRecycleBase *pRename = (rtnLTRecycleBase *)( _taskPtr.get() ) ;
      pmdEDUCB *cb = (pmdEDUCB *)pExe ;

      const utilRecycleItem &recycleItem = pRename->getRecycleItem() ;
      const CHAR *originName = recycleItem.getOriginName() ;
      const CHAR *recycleName = recycleItem.getRecycleName() ;

      BOOLEAN isRemoteItemExist = FALSE,
              isLocalItemExist = FALSE,
              isRemoteCSExist = FALSE,
              isLocalCSExist = FALSE,
              isLocalRecyCSExist = FALSE ;

      rc = _checkCSWithRecyItem( recycleItem, pExe,
                                 isRemoteItemExist,
                                 isRemoteCSExist,
                                 isLocalItemExist,
                                 isLocalCSExist,
                                 isLocalRecyCSExist ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check collection with recycle item "
                   "[origin: %s, recycle: %s], rc: %d",
                   originName, recycleName, rc ) ;

      if ( isRemoteItemExist )
      {
         PD_LOG( PDWARNING, "Unknown status of return collection space "
                 "item [origin:%s, recycle %s]", originName, recycleName ) ;
      }
      else
      {
         // need finish the job
         if ( !isLocalCSExist && isLocalRecyCSExist )
         {
            // return collection space
            dmsReturnOptions options ;
            options._recycleItem = recycleItem ;

            rc = rtnReturnCommand( options, cb, dmsCB, dpsCB, FALSE ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to return collection space "
                         "[%s] to [%s], rc: %d", recycleName, originName, rc ) ;

            // start index tasks
            sdbGetClsCB()->startIdxTaskCheckByCS(
                  (utilCSUniqueID)( recycleItem.getOriginID() ) ) ;
         }
         else if ( isLocalItemExist )
         {
            // delete item only
            rc = _recycleBinMgr->deleteItem( recycleItem, cb ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to delete recycle item "
                         "[origin %s, recycle %s], rc: %d", originName,
                         recycleName, rc ) ;
         }
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsRenameCheckJob::_doRenameRecycleCL( IExecutor *pExe )
   {
      INT32 rc = SDB_OK ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      rtnLTRecycleBase *pRename = (rtnLTRecycleBase *)( _taskPtr.get() ) ;
      pmdEDUCB *cb = (pmdEDUCB *)pExe ;

      BOOLEAN isRemoteItemExist = FALSE,
              isLocalItemExist = FALSE,
              isRemoteCLExist = FALSE,
              isLocalCLExist = FALSE,
              isLocalRecyCLExist = FALSE ;

      const utilRecycleItem &recycleItem = pRename->getRecycleItem() ;
      SDB_ASSERT( recycleItem.isValid(), "recycle item should be valid" ) ;
      const CHAR *originName = recycleItem.getOriginName() ;
      const CHAR *recycleName = recycleItem.getRecycleName() ;

      rc = _checkCLWithRecyItem( recycleItem, pExe,
                                 isRemoteItemExist,
                                 isRemoteCLExist,
                                 isLocalItemExist,
                                 isLocalCLExist,
                                 isLocalRecyCLExist ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check collection with recycle item "
                   "[origin: %s, recycle: %s], rc: %d",
                   originName, recycleName, rc ) ;

      if ( isRemoteItemExist )
      {
         // finish the job
         if ( isLocalCLExist && !isLocalRecyCLExist )
         {
            switch ( recycleItem.getOpType() )
            {
               case UTIL_RECYCLE_OP_DROP :
               {
                  dmsDropCLOptions options( recycleItem, FALSE ) ;
                  rc = rtnDropCollectionCommand( originName, cb, dmsCB, dpsCB,
                                                 UTIL_UNIQUEID_NULL, &options ) ;
                  PD_RC_CHECK( rc, PDWARNING, "Failed to recycle drop "
                               "collection [%s] to [%s], rc: %d", originName,
                               recycleName, rc ) ;
                  break ;
               }
               case UTIL_RECYCLE_OP_TRUNCATE :
               {
                  dmsTruncCLOptions options( recycleItem, FALSE ) ;
                  rc = rtnTruncCollectionCommand( originName, cb, dmsCB, dpsCB,
                                                  NULL, &options ) ;
                  PD_RC_CHECK( rc, PDWARNING, "Failed to recycle truncate "
                               "collection [%s] to [%s], rc: %d", originName,
                               recycleName, rc ) ;
                  break ;
               }
               default :
               {
                  SDB_ASSERT( FALSE, "invalid op type" ) ;
                  PD_CHECK( FALSE, SDB_SYS, error, PDERROR,
                            "Failed to finish recycle collection drop, "
                            "invalid op type [%d]", recycleItem.getOpType() ) ;
                  break ;
               }
            }
         }

         if ( !isLocalItemExist )
         {
            // save item
            rc = _recycleBinMgr->saveItem( recycleItem, cb ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to save recycle item "
                         "[origin %s, recycle %s], rc: %d", originName,
                         recycleName, rc ) ;
         }
      }
      else
      {
         if ( isLocalItemExist || isLocalRecyCLExist )
         {
            // drop item and recycled collection
            BOOLEAN isDropped = FALSE ;
            rc = _recycleBinMgr->dropItemWithCheck( recycleItem, cb, FALSE,
                                                    isDropped ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to drop recycle item "
                         "[origin %s, recycle %s], rc: %d", originName,
                         recycleName, rc ) ;
         }
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsRenameCheckJob::_doRenameReturnCL( IExecutor *pExe )
   {
      INT32 rc = SDB_OK ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      rtnLTRecycleBase *pRename = (rtnLTRecycleBase *)( _taskPtr.get() ) ;
      pmdEDUCB *cb = (pmdEDUCB *)pExe ;

      BOOLEAN isRemoteItemExist = FALSE,
              isLocalItemExist = FALSE,
              isRemoteCLExist = FALSE,
              isLocalCLExist = FALSE,
              isLocalRecyCLExist = FALSE ;

      const utilRecycleItem &recycleItem = pRename->getRecycleItem() ;
      SDB_ASSERT( recycleItem.isValid(), "recycle item should be valid" ) ;
      const CHAR *originName = recycleItem.getOriginName() ;
      const CHAR *recycleName = recycleItem.getRecycleName() ;

      rc = _checkCLWithRecyItem( recycleItem, pExe,
                                 isRemoteItemExist,
                                 isRemoteCLExist,
                                 isLocalItemExist,
                                 isLocalCLExist,
                                 isLocalRecyCLExist ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check collection with recycle item "
                   "[origin: %s, recycle: %s], rc: %d",
                   originName, recycleName, rc ) ;

      if ( isRemoteItemExist )
      {
         PD_LOG( PDWARNING, "Unknown status of return collection "
                 "item [origin:%s, recycle %s]", originName, recycleName ) ;
      }
      else
      {
         // need finish the job
         if ( !isLocalCLExist && isLocalRecyCLExist )
         {
            // return collection space
            dmsReturnOptions options ;
            options._recycleItem = recycleItem ;

            rc = rtnReturnCommand( options, cb, dmsCB, dpsCB, FALSE ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to return collection "
                         "[%s] to [%s], rc: %d", recycleName, originName, rc ) ;

            // start index tasks
            sdbGetClsCB()->startIdxTaskCheckByCL(
                  (utilCLUniqueID)( recycleItem.getOriginID() ) ) ;
         }
         else if ( isLocalItemExist )
         {
            // delete item only
            rc = _recycleBinMgr->deleteItem( recycleItem, cb ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to delete recycle item "
                         "[origin %s, recycle %s], rc: %d", originName,
                         recycleName, rc ) ;
         }
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsRenameCheckJob::_checkRemoteItem( const utilRecycleItem &recycleItem,
                                               IExecutor *pExe,
                                               BOOLEAN &isExist )
   {
      INT32 rc = SDB_OK ;

      pmdEDUCB *cb = (pmdEDUCB *)pExe ;

      shardCB *pShdMgr = sdbGetShardCB() ;
      const CHAR *recycleName = recycleItem.getRecycleName() ;
      utilRecycleID recycleID = recycleItem.getRecycleID() ;
      utilRecycleItem remoteItem ;

      rc = pShdMgr->rGetRecycleItem( cb, recycleID, remoteItem ) ;
      if ( SDB_RECYCLE_ITEM_NOTEXIST == rc )
      {
         isExist = FALSE ;
         rc = SDB_OK ;
      }
      else if ( SDB_OK == rc )
      {
         isExist = TRUE ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get recycle item [%s] "
                   "from CATALOG, rc: %d", recycleName, rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsRenameCheckJob::_checkLocalItem( const utilRecycleItem &recycleItem,
                                              IExecutor *pExe,
                                              BOOLEAN &isExist )
   {
      INT32 rc = SDB_OK ;

      pmdEDUCB *cb = (pmdEDUCB *)pExe ;

      const CHAR *recycleName = recycleItem.getRecycleName() ;
      utilRecycleItem localItem ;

      rc = _recycleBinMgr->getItem( recycleName, cb, localItem ) ;
      if ( SDB_RECYCLE_ITEM_NOTEXIST == rc )
      {
         isExist = FALSE ;
         rc = SDB_OK ;
      }
      else if ( SDB_OK == rc )
      {
         isExist = TRUE ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get recycle item [%s], "
                   "rc: %d", recycleName, rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsRenameCheckJob::_checkLocalCL( const CHAR *csName,
                                            const CHAR *clShortName,
                                            utilCLUniqueID clUniqueID,
                                            BOOLEAN &isExist )
   {
      INT32 rc = SDB_OK ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      _dmsStorageUnit *su = NULL ;
      UINT16 mbID = DMS_INVALID_MBID ;

      /// get local cl unique id
      rc = dmsCB->nameToSUAndLock( csName,  suID,  &su ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         isExist = FALSE ;
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to lock collection space [%s], "
                   "rc: %d", csName, rc ) ;

      if ( NULL != su )
      {
         utilCLUniqueID localCLUID = UTIL_UNIQUEID_NULL ;

         rc = su->data()->findCollection( clShortName, mbID, &localCLUID ) ;
         if ( SDB_DMS_NOTEXIST == rc )
         {
            isExist = FALSE ;
            rc = SDB_OK ;
         }
         else if ( SDB_OK == rc )
         {
            if ( ( UTIL_UNIQUEID_NULL == clUniqueID ) ||
                 ( localCLUID == clUniqueID ) )
            {
               isExist = TRUE ;
            }
            else
            {
               isExist = FALSE ;
            }
         }
         PD_RC_CHECK( rc, PDWARNING, "Failed to get collection [%s] from "
                      "collection space [%s], rc: %d", clShortName, csName,
                      rc ) ;
      }

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
         suID = DMS_INVALID_SUID ;
      }
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsRenameCheckJob::_checkLocalCS( const CHAR *csName,
                                            utilCSUniqueID csUniqueID,
                                            BOOLEAN &isExist )
   {
      INT32 rc = SDB_OK ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      _dmsStorageUnit *su = NULL ;

      /// test from cs
      rc = dmsCB->nameToSUAndLock( csName, suID, &su ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         isExist = FALSE ;
         rc = SDB_OK ;
         goto done ;
      }
      else if ( SDB_OK == rc )
      {
         if ( ( UTIL_UNIQUEID_NULL == csUniqueID ) ||
              ( su->CSUniqueID() == csUniqueID ) )
         {
            isExist = TRUE ;
         }
         else
         {
            isExist = FALSE ;
         }
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to lock collection space [%s], "
                   "rc: %d", csName, rc ) ;

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
         suID = DMS_INVALID_SUID ;
      }
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsRenameCheckJob::_checkRemoteCL( const CHAR *csName,
                                             const CHAR *clShortName,
                                             utilCLUniqueID clUniqueID,
                                             BOOLEAN &isExist )
   {
      INT32 rc = SDB_OK ;

      shardCB *pShdMgr = sdbGetShardCB() ;

      clsCatalogSet *pSet = NULL ;
      UINT32 groupCount = 0 ;

      CHAR clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
      ossSnprintf( clFullName, DMS_COLLECTION_FULL_NAME_SZ, "%s.%s",
                   csName, clShortName ) ;

      BOOLEAN needClear = FALSE ;

      /// clear local catalog info
      pShdMgr->getCataAgent()->lock_w() ;
      pShdMgr->getCataAgent()->clear( clFullName ) ;
      pShdMgr->getCataAgent()->release_w() ;

      /// Get to cl info
      rc = pShdMgr->getAndLockCataSet( clFullName, &pSet ) ;
      if ( SDB_DMS_NOTEXIST == rc ||
           SDB_DMS_CS_NOTEXIST == rc )
      {
         isExist = FALSE ;
         rc = SDB_OK ;
      }
      else if ( SDB_OK == rc )
      {
         if ( 0 == groupCount )
         {
            isExist = FALSE ;
            needClear = TRUE ;
         }
         else if ( clUniqueID != pSet->clUniqueID() )
         {
            isExist = FALSE ;
         }
         else
         {
            isExist = TRUE ;
         }
         pShdMgr->unlockCataSet( pSet ) ;
      }
      PD_RC_CHECK( rc, PDINFO, "Failed to get catalog information for "
                   "collection [%s], rc: %d", clFullName, rc ) ;

      if ( needClear )
      {
         /// The collection is not on the group
         pShdMgr->getCataAgent()->lock_w() ;
         pShdMgr->getCataAgent()->clear( clFullName ) ;
         pShdMgr->getCataAgent()->release_w() ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsRenameCheckJob::_checkRemoteCS( const CHAR *csName,
                                             utilCSUniqueID localCSUID,
                                             BOOLEAN &isExist )
   {
      INT32 rc = SDB_OK ;

      shardCB *pShdMgr = sdbGetShardCB() ;

      utilCSUniqueID remoteCSUID = UTIL_UNIQUEID_NULL ;

      /// Get to cs info
      rc = pShdMgr->rGetCSInfo( csName, remoteCSUID ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         isExist = FALSE ;
         rc = SDB_OK ;
      }
      else if ( SDB_OK == rc )
      {
         if ( remoteCSUID != localCSUID )
         {
            isExist = FALSE ;
         }
         else
         {
            isExist = TRUE ;
         }
      }
      PD_RC_CHECK( rc, PDINFO, "Failed to get catalog information for "
                   "collection space [%s], rc: %d", csName, rc ) ;

   done:
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
              RTN_LOCAL_TASK_RENAMECL == taskPtr->getTaskType() ||
              RTN_LOCAL_TASK_RECYCLECS == taskPtr->getTaskType() ||
              RTN_LOCAL_TASK_RECYCLECL == taskPtr->getTaskType() ||
              RTN_LOCAL_TASK_RETURNCS == taskPtr->getTaskType() ||
              RTN_LOCAL_TASK_RETURNCL == taskPtr->getTaskType() )
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
