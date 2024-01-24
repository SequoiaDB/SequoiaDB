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

   Source File Name = monClass.cpp

   Descriptive Name = Monitor Class source

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/14/2019  CW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "monLatch.hpp"
#include "pmd.hpp"

namespace engine
{
#define MON_GET_LATCH_LVL ( g_monMgrPtr \
                       ->getCollectionLvl(MON_CLASS_LATCH) )

const CHAR* monLatchName[] =
{
   "",
   "SDB_DMSCB stateMtx",
   "dmsStorageBase persistLatch",
   "dmsStorageBase commitLatch",
   "dmsStorageDataCommon latchContext",
   "dpsTransCB MapMutex",
   "dpsTransCB CBMapMutex",
   "dpsTransCB lsnMapMutex",
   "dpsTransCB hisMutex",
   "SDB_RTNCB mutex",
   "dmsStorageDataCommon mblock",
   "catGTSMsgHandler jobLatch",
   "catMainController contextLatch",
   "catSequence latch",
   "clsShardingKeySite mutex",
   "clsDCMgr peerCatLatch",
   "clsDataSrcBaseSession LSNlatch",
   "clsBucket bucketLatch",
   "clsFreezingWindow latch",
   "clsShardMgr catLatch",
   "CoordCB contextLatch",
   "dmsSegmentSpace mutex",
   "dmsPageMappingDispatcher latch",
   "dmsStorageLob delayOpenLatch",
   "dpsArchiveInfoMgr mutex",
   "dpsArchiveMgr mutex",
   "dpsReplicaLogMgr mtx",
   "dpsReplicaLogMgr writeMutex",
   "dpsTransCB maxFileSizeMutex",
   "dpsTransLRBHash maxFileSizeMutex",
   "netEventHandler mtx",
   "omManager contextLatch",
   "omStrategyMgr contextLatch",
   "omAgentMgr scopeLatch",
   "optCachedPlanActivity latch",
   "optAccessPlanManager reinitLatch",
   "pmdSessionMeta Latch",
   "pmdAsycSessionMgr metaLatch",
   "pmdAsycSessionMgr deqDeletingMutex",
   "pmdAsycSessionMgr forceLatch",
   "iPmdDMNChildProc mutex",
   "pmdLightJobMgr unitLatch",
   "pmdBuffPool unitLatch",
   "pmdCfgRecord mutex",
   "restSessionInfo inLatch",
   "pmdSyncMgr unitLatch",
   "rtnExtDataHandler latch",
   "rtnLobAccessInfo lock",
   "schedTaskContanierMgr latch",
   "schedTaskMgr latch",
   "spdFMPMgr mtx",
   "utilCacheUnit pageCleaner",
   "utilMemBlockPool latch",
   "utilSegmentPool latch",
   "catDCLogMgr latch",
   "clsMgr clsLatch",
   "clsShardMgr shardLatch",
   "clsTaskMgr taskLatch",
   "clsTaskMgr regLatch",
   "coordOmStrategyAgent latch",
   "coordResource nodeMutex",
   "coordResource cataMutex",
   "SDB_DMSCB mutex",
   "dmsStorageBase segmentLatch",
   "dmsStorageDataCommon metadataLatch",
   "dmsSysSUMgr mutex",
   "oldVersionCB oldVersionCBLatch",
   "netEHSegment mtx",
   "netFrame suiteMtx",
   "netFrame mtx",
   "netRoute mtx",
   "omManager omLatch",
   "omHostVersion lock",
   "omaAsyncTask planLatch",
   "omAgentOptions latch",
   "omAgentMgr immediatelyTimerLatch",
   "omAgentMgr mgrLatch",
   "omAgentNodeMgr mapLatch",
   "omAgentNodeMgr guardLatch",
   "omaAddHostTask taskLatch",
   "omaRemoveHostTask taskLatch",
   "omaInstDBBusTask taskLatch",
   "omaRemoveDBBusTask taskLatch",
   "omaZNBusTaskBase taskLatch",
   "omaSsqlOlapBusBase taskLatch",
   "omaSsqlExecTask taskLatch",
   "omaTask latch",
   "omaTaskMgr taskLatch",
   "ossASIO mutex",
   "OSS_FILE mutex",
   "SDB_KRCB handlerLatch",
   "pmdController ctrlLatch",
   "pmdEDUCB mutex",
   "pmdEDUMgr latch",
   "pmdRemoteSessionMgr edusLatch",
   "rtnJobMgr latch",
   "rtnJobMgr latchRemove",
   "rtnExtDataProcessorMgr mutex",
   "clsCatalogAgent rwMutex",
   "clsGroupItem rwMutex",
   "clsNodeMgrAgent rwMutex",
   "clsDCBaseInfo rwMutex",
   "clsGroupInfo rwMutex",
   "clsBucket counterLock",
   "clsReplicateSet vecLatch",
   "dmsCompressorEntry lock",
   "dmsSMEMgr mutex",
   "dpsLogPage mtx;",
   "dpsTransLockManager rwMutex",
   "netEventSuit rwMutex",
   "netFrame suiteExitMutex",
   "optCachedPlanMonitor clearLock",
   "ossMmapSegment rwMutex",
   "pmdEDUMgr eduExitMutex",
   "rtnContextBase dataLock",
   "rtnContextBase prefetchLock",
   "rtnRemoteMessenger lock",
   "utilCacheBucket rwMutex",
   "utilHashTable bucketNumLock",
} ;

const CHAR* monLatchIDtoName ( MON_LATCH_IDENTIFIER latchID)
{
   if ( (latchID < 1) || (latchID >= MON_LATCH_ID_MAX) )
   {
      return monLatchName[0] ;
   }
   else
   {
      return monLatchName[latchID] ;
   }
}

template <class T>
void _monGetLatch(T* latchObj);

template <class T>
void _monGetLatch(T* latchObj)
{
#if defined (SDB_ENGINE)
   if ( MON_GET_LATCH_LVL != MON_DATA_LVL_NONE )
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      if ( FALSE == latchObj->latch.try_get() )
      {
         monClassLatch *monLatchCB = NULL ;

         ossTick begin, end ;
         begin.sample() ;

         if ( MON_GET_LATCH_LVL == MON_DATA_LVL_DETAIL )
         {
            monClassLatchData data;
            data.latchID = latchObj->latchID ;

            if ( cb )
            {
               data.waiterTID = cb->getTID() ;
            }
            data.ownerTID = latchObj->xOwnerTID ;
            data.latchAddr = (void *) latchObj ;
            data.latchMode = EXCLUSIVE ;
            data.numOwner = latchObj->getNumOwner() ;
            data.lastSOwner = latchObj->lastSOwnerTID ;

            monLatchCB = g_monMgrPtr->registerMonitorObject<monClassLatch>(&data) ;
         }

         latchObj->latch.get() ;

         if ( cb )
         {
            latchObj->xOwnerTID = cb->getTID() ;
         }

         end.sample() ;

         if ( monLatchCB )
         {
            monLatchCB->waitTime += end - begin ;
            g_monMgrPtr->removeMonitorObject( monLatchCB ) ;
         }

         if (cb && cb->getMonQueryCB() )
         {
           cb->getMonQueryCB()->latchWaitTime += end - begin ;
         }
      }
      else
      {
         if ( cb )
         {
            latchObj->xOwnerTID = cb->getTID() ;
         }
      }
   }
   else
   {
     latchObj->latch.get() ;
   }
   latchObj->numXOwner = 1 ;
#else
   latchObj->latch.get() ;
#endif
}

template <class T>
void _monGetSLatch(T* latchObj);

template <class T>
void _monGetSLatch(T* latchObj)
{
#if defined (SDB_ENGINE)
   if ( MON_GET_LATCH_LVL != MON_DATA_LVL_NONE )
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      if ( FALSE == latchObj->latch.try_get_shared() )
      {
         monClassLatch *monLatchCB = NULL ;

         ossTick begin, end ;
         begin.sample() ;

         if ( MON_GET_LATCH_LVL == MON_DATA_LVL_DETAIL )
         {
            monClassLatchData data;
            data.latchID = latchObj->latchID ;
            if ( cb )
            {
               data.waiterTID = cb->getTID() ;
            }
            data.ownerTID = latchObj->xOwnerTID ;
            data.latchAddr = (void *)latchObj ;
            data.latchMode = SHARED ;
            data.numOwner = latchObj->getNumOwner() ;
            data.lastSOwner = latchObj->lastSOwnerTID ;

            monLatchCB = g_monMgrPtr->registerMonitorObject<monClassLatch>(&data) ;
         }

         latchObj->latch.get_shared() ;

         if ( cb )
         {
            latchObj->lastSOwnerTID = cb->getTID() ;
         }

         end.sample() ;

         if ( monLatchCB )
         {
            monLatchCB->waitTime += end - begin ;
            g_monMgrPtr->removeMonitorObject( monLatchCB ) ;
         }

         if (cb && cb->getMonQueryCB() )
         {
           cb->getMonQueryCB()->latchWaitTime += end - begin ;
         }
      }
      else
      {
         if ( cb )
         {
            latchObj->lastSOwnerTID = cb->getTID() ;
         }
      }
   }
   else
   {
     latchObj->latch.get_shared() ;
   }
   latchObj->numSOwner.inc() ;
#else
   latchObj->latch.get_shared() ;
#endif

}

monSpinXLatch::monSpinXLatch( MON_LATCH_IDENTIFIER latchID )
   : lastSOwnerTID( 0 ),
     xOwnerTID( 0 ),
     numXOwner( 0 )
{
   this->latchID = latchID ;
}

monSpinXLatch::~monSpinXLatch()
{
}

void monSpinXLatch::get()
{
   _monGetLatch<monSpinXLatch>(this) ;
}

void monSpinXLatch::release()
{
   latch.release() ;
   numXOwner = 0 ;
   xOwnerTID = 0 ;
}

BOOLEAN monSpinXLatch::try_get()
{
   BOOLEAN ret = latch.try_get() ;
#if defined (SDB_ENGINE)
   if ( ret )
   {
      numXOwner = 1 ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb )
      {
         xOwnerTID = cb->getTID() ;
      }
   }
#endif
   return ret ;
}

INT32 monSpinXLatch::getNumOwner()
{
   return numXOwner ;
}

/**
 * monSpinSLatch implements
 */

monSpinSLatch::monSpinSLatch( MON_LATCH_IDENTIFIER latchID )
   : lastSOwnerTID ( 0 ),
     xOwnerTID ( 0 ),
     numSOwner( 0 ),
     numXOwner( 0 )
{
   this->latchID = latchID ;
}

monSpinSLatch::monSpinSLatch()
   : lastSOwnerTID ( 0 ),
     xOwnerTID ( 0 ),
     numSOwner( 0 ),
     numXOwner( 0 )
{
}

monSpinSLatch::~monSpinSLatch()
{
}

void monSpinSLatch::get()
{
   _monGetLatch<monSpinSLatch>( this ) ;
}

void monSpinSLatch::release()
{
   latch.release() ;
   xOwnerTID = 0 ;
   numXOwner = 0 ;
}

void monSpinSLatch::get_shared ()
{
   _monGetSLatch<monSpinSLatch>( this ) ;
}

void monSpinSLatch::release_shared ()
{
   latch.release_shared() ;
   numSOwner.dec() ;
}

BOOLEAN monSpinSLatch::try_get_shared()
{
   BOOLEAN ret = latch.try_get_shared() ;
#if defined (SDB_ENGINE)
   if ( ret )
   {
      numSOwner.inc() ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb )
      {
         lastSOwnerTID = cb->getTID() ;
      }
   }
#endif
   return ret ;
}

BOOLEAN monSpinSLatch::try_get()
{
   BOOLEAN ret = latch.try_get() ;
#if defined (SDB_ENGINE)
   if ( ret )
   {
      numXOwner = 1 ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb )
      {
         xOwnerTID = cb->getTID() ;
      }
   }
#endif
   return ret ;
}

INT32 monSpinSLatch::getNumOwner()
{
   return numSOwner.fetch() + numXOwner ;
}

/**
 * monRWMutex implements
 */

monRWMutex::monRWMutex( MON_LATCH_IDENTIFIER latchID, UINT32 type )
: mutex( type ),
  lastSOwnerTID ( 0 ),
  xOwnerTID ( 0 ),
  numSOwner( 0 ),
  numXOwner( 0 )
{
   this->latchID = latchID ;
}

monRWMutex::~monRWMutex()
{
}

INT32 monRWMutex::lock_r( INT32 millisec )
{
   INT32 rc = SDB_OK ;
#if defined (SDB_ENGINE)
   if ( MON_GET_LATCH_LVL != MON_DATA_LVL_NONE )
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      if ( FALSE == this->mutex.try_lock_r() )
      {
         monClassLatch *monLatchCB = NULL ;

         ossTick begin, end ;
         begin.sample() ;

         if ( MON_GET_LATCH_LVL == MON_DATA_LVL_DETAIL )
         {
            monClassLatchData data;
            data.latchID = this->latchID ;
            if ( cb )
            {
               data.waiterTID = cb->getTID() ;
            }
            data.ownerTID = this->xOwnerTID ;
            data.latchAddr = (void *)this ;
            data.latchMode = SHARED ;
            data.numOwner = this->getNumOwner() ;
            data.lastSOwner = this->lastSOwnerTID ;

            monLatchCB = g_monMgrPtr->registerMonitorObject<monClassLatch>(&data) ;
         }

         rc = this->mutex.lock_r(millisec) ;

         if ( cb && SDB_OK == rc )
         {
            this->lastSOwnerTID = cb->getTID() ;
         }

         end.sample() ;

         if ( monLatchCB )
         {
            monLatchCB->waitTime += end - begin ;
            g_monMgrPtr->removeMonitorObject( monLatchCB ) ;
         }

         if (cb && cb->getMonQueryCB() )
         {
            cb->getMonQueryCB()->latchWaitTime += end - begin ;
         }
      }
      else
      {
         if ( cb )
         {
            this->lastSOwnerTID = cb->getTID() ;
         }
      }
   }
   else
   {
     rc = this -> mutex.lock_r(millisec) ;
   }
   if ( SDB_OK == rc ) this->numSOwner.inc() ;
#else
   rc = this->mutex.lock_r(millisec) ;
#endif
   return rc;
}

INT32 monRWMutex::release_r()
{
   INT32 rc = mutex.release_r() ;

   if ( SDB_OK == rc )
   {
      numSOwner.dec() ;
   }
   return rc ;
}

INT32 monRWMutex::lock_w( INT32 millisec )
{
   INT32 rc = SDB_OK ;
#if defined (SDB_ENGINE)
   if ( MON_GET_LATCH_LVL != MON_DATA_LVL_NONE )
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      if ( FALSE == this->mutex.try_lock_w() )
      {
         monClassLatch *monLatchCB = NULL ;

         ossTick begin, end ;
         begin.sample() ;

         if ( MON_GET_LATCH_LVL == MON_DATA_LVL_DETAIL )
         {
            monClassLatchData data;
            data.latchID = this->latchID ;

            if ( cb )
            {
               data.waiterTID = cb->getTID() ;
            }
            data.ownerTID = this->xOwnerTID ;
            data.latchAddr = (void *) this ;
            data.latchMode = EXCLUSIVE ;
            data.numOwner = this->getNumOwner() ;
            data.lastSOwner = this->lastSOwnerTID ;

            monLatchCB = g_monMgrPtr->registerMonitorObject<monClassLatch>(&data) ;
         }

         rc = this->mutex.lock_w(millisec) ;
         if ( cb  && SDB_OK == rc )
         {
            this->xOwnerTID = cb->getTID() ;
         }

         end.sample() ;

         if ( monLatchCB )
         {
            monLatchCB->waitTime += end - begin ;
            g_monMgrPtr->removeMonitorObject( monLatchCB ) ;
         }

         if (cb && cb->getMonQueryCB() )
         {
            cb->getMonQueryCB()->latchWaitTime += end - begin ;
         }
      }
      else
      {
         if ( cb )
         {
            this->xOwnerTID = cb->getTID() ;
         }
      }
   }
   else
   {
      rc = this->mutex.lock_w(millisec) ;
   }
   if ( SDB_OK == rc ) this->numXOwner = 1 ;
#else
   rc = this->mutex.lock_w(millisec) ;
#endif
   return rc;
}

INT32 monRWMutex::release_w()
{
   INT32 rc = mutex.release_w() ;

   if ( SDB_OK == rc )
   {
      xOwnerTID = 0 ;
      numXOwner = 0 ;
   }

   return rc ;
}

BOOLEAN monRWMutex::try_lock_r()
{
   BOOLEAN ret = mutex.try_lock_r() ;
#if defined (SDB_ENGINE)
   if ( ret )
   {
      numSOwner.inc() ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb )
      {
         lastSOwnerTID = cb->getTID() ;
      }
   }
#endif
   return ret ;

}

BOOLEAN monRWMutex::try_lock_w()
{
   BOOLEAN ret = mutex.try_lock_w() ;
#if defined (SDB_ENGINE)
   if ( ret )
   {
      numXOwner = 1 ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb )
      {
         xOwnerTID = cb->getTID() ;
      }
   }
#endif
   return ret ;
}

INT32 monRWMutex::getNumOwner()
{
   return numSOwner.fetch() + numXOwner ;
}
}
