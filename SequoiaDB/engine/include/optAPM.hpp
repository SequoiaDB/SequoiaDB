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

   Source File Name = optAPM.hpp

   Descriptive Name = Optimizer Access Plan Manager Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Access
   Plan Manager, which is pooling access plans that has been used.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft
          01/07/2017  HGM Move from rtnAPM.hpp

   Last Changed =

*******************************************************************************/
#ifndef OPTAPM_HPP__
#define OPTAPM_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "optAccessPlan.hpp"
#include "optAccessPlanRuntime.hpp"
#include "utilHashTable.hpp"
#include "dmsCachedPlanUnit.hpp"
#include "dmsEventHandler.hpp"

using namespace std ;

namespace engine
{

   #define OPT_PLAN_CACHE_DFT_LOCK_NUM    ( 64 )
   #define OPT_PLAN_CACHE_ACT_HIGH_PERC   ( 0.80 )
   #define OPT_PLAN_CACHE_ACT_LOW_PERC    ( 0.50 )
   #define OPT_PLAN_CACHE_AVG_BUCKET_SIZE ( 3 )

   class _optCachedPlanMonitor ;
   typedef class _optCachedPlanMonitor optCachedPlanMonitor ;

   /*
      _optAccessPlanCache define
    */
   class _optAccessPlanCache : public _utilHashTable< optAccessPlanKey,
                                                      optAccessPlan,
                                                      OPT_PLAN_CACHE_DFT_LOCK_NUM >
   {
      public :
         _optAccessPlanCache () ;

         ~_optAccessPlanCache () ;

         BOOLEAN initialize ( UINT32 bucketNum,
                              optCachedPlanMonitor *pMonitor ) ;

         void deinitialize () ;

         BOOLEAN addPlan ( optAccessPlan *pPlan ) ;

         void removeCachedPlan ( optAccessPlan *pPlan, INT32 lockType = -1 ) ;

         void resetCachedPlanActivity( optAccessPlan *pPlan,
                                       INT32 lockType = -1 ) ;

         void invalidateSUPlans ( dmsCachedPlanMgr *pCachedPlanMgr,
                                  UINT32 suLID ) ;

         void invalidateCLPlans ( dmsCachedPlanMgr *pCachedPlanMgr,
                                  UINT32 suLID, UINT32 clLID ) ;

         void invalidateAllPlans () ;

         void invalidateCLPlans ( const CHAR *pCLFullName ) ;

         void invalidateSUPlans ( const CHAR *pCSName ) ;

         UINT32 getCachedPlanCount () const ;

         INT32 getCachedPlanList ( vector<BSONObj> &cachedPlanList ) ;

         void enableCaching () ;

         void disableCaching () ;

      protected :
         virtual void _afterAddItem ( UINT32 bucketID, optAccessPlan *pPlan ) ;

         virtual void _afterGetItem ( UINT32 bucketID, optAccessPlan *pPlan ) ;

         virtual void _afterRemoveItem ( UINT32 bucketID, optAccessPlan *pPlan ) ;

      protected :
         optCachedPlanMonitor *_pMonitor ;
   } ;

   typedef class _optAccessPlanCache optAccessPlanCache ;

   /*
      _optCachedPlanActivity define
    */
   class _optCachedPlanActivity : public SDBObject
   {
      public :
         _optCachedPlanActivity () ;

         ~_optCachedPlanActivity () ;

         void clear () ;

         void setPlan ( optAccessPlan *pPlan, UINT64 timestamp ) ;

         OSS_INLINE optAccessPlan *getPlan ()
         {
            return _pPlan ;
         }

         OSS_INLINE UINT64 getLastAccessTime () const
         {
            return _lastAccessTime ;
         }

         OSS_INLINE void setLastAccessTime ( UINT64 lastAccessTime )
         {
            _lastAccessTime = lastAccessTime ;
         }

         OSS_INLINE UINT64 getAccessCount () const
         {
            return _accessCount ;
         }

         OSS_INLINE void incAccessCount ()
         {
            _accessCount ++ ;
         }

         OSS_INLINE UINT64 getPeriodAccessCount () const
         {
            return _periodAccessCount ;
         }

         OSS_INLINE void incPeriodAccessCount ()
         {
            _periodAccessCount ++ ;
         }

         OSS_INLINE void decPeriodAccessCount ( UINT64 count )
         {
            if ( _periodAccessCount > count )
            {
               _periodAccessCount -= count ;
            }
            else
            {
               _periodAccessCount = 0 ;
            }
         }

         OSS_INLINE BOOLEAN isEmpty () const
         {
            return ( NULL == _pPlan ) ;
         }

         void setQueryActivity ( const optQueryActivity &queryActivity,
                                 const rtnParamList &parameters ) ;

         void toBSON ( BSONObjBuilder &builder ) ;

      protected:
         void _clearPlan()
         {
            if ( NULL != _pPlan )
            {
               _pPlan->release() ;
               _pPlan = NULL ;
            }
         }

         void _setPlan( optAccessPlan *pPlan )
         {
            _clearPlan() ;
            if ( NULL != pPlan )
            {
               pPlan->incRefCount() ;
               _pPlan = pPlan ;
            }
         }

      protected :
         optAccessPlan *   _pPlan ;
         UINT64            _lastAccessTime ;
         UINT64            _periodAccessCount ;
         UINT64            _accessCount ;
         ossTickDelta      _totalQueryTimeTick ;
         optQueryActivity  _maxQueryActivity ;
         optQueryActivity  _minQueryActivity ;

         monSpinXLatch     _latch ;
   } ;

   typedef class _optCachedPlanActivity optCachedPlanActivity ;

   /*
      _optCachedPlanMonitor define
    */
   class _optCachedPlanMonitor : public SDBObject
   {
      public :
         _optCachedPlanMonitor () ;

         ~_optCachedPlanMonitor () ;

         BOOLEAN initialize ( optAccessPlanCache *pPlanCache ) ;

         void deinitialize () ;

         OSS_INLINE BOOLEAN isInitialized () const
         {
            return ( NULL != _pActivities ) ;
         }

         BOOLEAN setActivity ( optAccessPlan *pPlan ) ;

         OSS_INLINE void setCachedPlanActivity ( optAccessPlan *pPlan )
         {
            INT32 activityID = pPlan->getActivityID() ;
            if ( OPT_INVALID_ACT_ID != activityID )
            {
               optCachedPlanActivity &activity = _pActivities[ activityID ] ;
               activity.setLastAccessTime( _accessTimestamp.inc() ) ;
               activity.incPeriodAccessCount() ;
            }
         }

         OSS_INLINE optCachedPlanActivity *getActivity ( INT32 activityID )
         {
            if ( OPT_INVALID_ACT_ID != activityID )
            {
               return &( _pActivities[ activityID ] ) ;
            }
            return NULL ;
         }

         OSS_INLINE void resetActivity ( INT32 activityID )
         {
            if ( OPT_INVALID_ACT_ID != activityID )
            {
               _pActivities[ activityID ].setPlan( NULL, 0 ) ;
               UINT64 freeActivityIndex = _freeIndexEnd.inc() % _activityNum ;
               _pFreeActivityIDs[ freeActivityIndex ] = activityID ;
               _cachedPlanCount.dec() ;
            }
         }

         OSS_INLINE UINT32 getCachedPlanCount () const
         {
            return _cachedPlanCount.peek() ;
         }

         OSS_INLINE monRWMutex *getClearLock ()
         {
            return &_clearLock ;
         }

         OSS_INLINE ossEvent *getClearEvent ()
         {
            return &_clearEvent ;
         }

         void signalPlanClearJob () ;

         void clearCachedPlans () ;

         void clearExpiredCachedPlans () ;

      protected :
         INT32 _allocateActivity ( optAccessPlan *pPlan ) ;

      protected :
         // Begin to the free index, where to get free activities
         ossAtomic64 _freeIndexBegin ;

         // End to the free index, where to return free activities
         ossAtomic64 _freeIndexEnd ;

         // Free index
         UINT32 *_pFreeActivityIDs ;

         // Thread flag to clearing procedure
         ossAtomic32 _clearThread ;

         // Mutex to protect clearing procedure
         monRWMutex _clearLock ;

         // Clear event to signal clear job
         ossEvent _clearEvent ;

         // Number of activities ( The capacity of plan cache )
         UINT32 _activityNum ;

         // High water mark to clear cached plans
         UINT32 _highWaterMark ;

         // Low water mark to stop clear cached plans
         UINT32 _lowWaterMark ;

         // Clock index to scan the activity table
         UINT32 _clockIndex ;

         // Activity table
         optCachedPlanActivity *_pActivities ;

         // Total number of cached plans
         ossAtomic32 _cachedPlanCount ;

         // Access timestamp for cached plans
         ossAtomic64 _accessTimestamp ;

         // Last timestamp to finished clearing procedure
         UINT64 _lastClearTimestamp ;

         // Pointer to plan cache
         optAccessPlanCache *_pPlanCache ;
   } ;

   /*
      _optAccessPlanManager define
    */
   class _optAccessPlanManager : public SDBObject,
                                 public _IDmsEventHandler,
                                 public _optAccessPlanConfigHolder,
                                 public _mthMatchConfigHolder
   {
      public :
         _optAccessPlanManager () ;

         ~_optAccessPlanManager () ;

         INT32 init ( UINT32 bucketNum,
                      OPT_PLAN_CACHE_LEVEL cacheLevel,
                      UINT32 sortBufferSize,
                      INT32 optCostThreshold,
                      BOOLEAN enableMixCmp,
                      INT32 planCacheMainCLThreshold ) ;

         INT32 reinit ( UINT32 bucketNum,
                        OPT_PLAN_CACHE_LEVEL cacheLevel,
                        UINT32 sortBufferSize,
                        INT32 optCostThreshold,
                        BOOLEAN enableMixCmp,
                        INT32 planCacheMainCLThreshold ) ;

         INT32 fini () ;

         OSS_INLINE OPT_PLAN_CACHE_LEVEL getCacheLevel () const
         {
            return _cacheLevel ;
         }

         OSS_INLINE BOOLEAN isInitialized () const
         {
            return _planCache.isInitialized() ;
         }

         OSS_INLINE optAccessPlanCache *getPlanCache ()
         {
            return &_planCache ;
         }

         OSS_INLINE optCachedPlanMonitor *getPlanMonitor ()
         {
            return &_monitor ;
         }

         // Try to get access plan from cache, if could not get access plan
         // from cache, create one
         INT32 getAccessPlan ( const rtnQueryOptions &options,
                               dmsStorageUnit *su,
                               dmsMBContext *mbContext,
                               optAccessPlanRuntime &planRuntime,
                               const rtnExplainOptions *expOptions = NULL ) ;

         // Create access plan directly without caching
         INT32 getTempAccessPlan ( const rtnQueryOptions &options,
                                   dmsStorageUnit *su,
                                   dmsMBContext *mbContext,
                                   optAccessPlanRuntime &planRuntime ) ;

         void invalidateCLPlans ( const CHAR *pCLFullName ) ;

         void invalidateSUPlans ( const CHAR * pCSName ) ;

         void invalidateAllPlans () ;

         void setQueryActivity ( INT32 activityID,
                                 const optQueryActivity &queryActivity,
                                 const rtnParamList &parameters ) ;

         // acquire access plan ID
         INT64 acquireAccessPlanID()
         {
            return _accessPlanIdGenerator.inc() ;
         }

      public :
         // For _IDmsEventHandler
         virtual INT32 onCreateCS ( IDmsEventHolder *pEventHolder,
                                    IDmsSUCacheHolder *pCacheHolder,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

         virtual INT32 onLoadCS ( IDmsEventHolder *pEventHolder,
                                  IDmsSUCacheHolder *pCacheHolder,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) ;

         virtual INT32 onUnloadCS ( IDmsEventHolder *pEventHolder,
                                    IDmsSUCacheHolder *pCacheHolder,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

         virtual INT32 onRenameCS ( IDmsEventHolder *pEventHolder,
                                    IDmsSUCacheHolder *pCacheHolder,
                                    const CHAR *pOldCSName,
                                    const CHAR *pNewCSName,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

         virtual INT32 onDropCS ( SDB_EVENT_OCCUR_TYPE type,
                                  IDmsEventHolder *pEventHolder,
                                  IDmsSUCacheHolder *pCacheHolder,
                                  const dmsEventSUItem &suItem,
                                  dmsDropCSOptions *options,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) ;

         virtual INT32 onRenameCL ( IDmsEventHolder *pEventHolder,
                                    IDmsSUCacheHolder *pCacheHolder,
                                    const dmsEventCLItem &clItem,
                                    const CHAR *pNewCLName,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

         virtual INT32 onTruncateCL ( SDB_EVENT_OCCUR_TYPE type,
                                      IDmsEventHolder *pEventHolder,
                                      IDmsSUCacheHolder *pCacheHolder,
                                      const dmsEventCLItem &clItem,
                                      dmsTruncCLOptions *options,
                                      pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB ) ;

         virtual INT32 onDropCL ( SDB_EVENT_OCCUR_TYPE type,
                                  IDmsEventHolder *pEventHolder,
                                  IDmsSUCacheHolder *pCacheHolder,
                                  const dmsEventCLItem &clItem,
                                  dmsDropCLOptions *options,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) ;

         virtual INT32 onRebuildIndex ( IDmsEventHolder *pEventHolder,
                                        IDmsSUCacheHolder *pCacheHolder,
                                        const dmsEventCLItem &clItem,
                                        const dmsEventIdxItem &idxItem,
                                        pmdEDUCB *cb,
                                        SDB_DPSCB *dpsCB ) ;

         virtual INT32 onDropIndex ( IDmsEventHolder *pEventHolder,
                                     IDmsSUCacheHolder *pCacheHolder,
                                     const dmsEventCLItem &clItem,
                                     const dmsEventIdxItem &idxItem,
                                     pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB ) ;

         virtual INT32 onClearSUCaches ( IDmsEventHolder *pEventHolder,
                                         IDmsSUCacheHolder *pCacheHolder ) ;

         virtual INT32 onClearCLCaches ( IDmsEventHolder *pEventHolder,
                                         IDmsSUCacheHolder *pCacheHolder,
                                         const dmsEventCLItem &clItem ) ;

         virtual INT32 onChangeSUCaches ( IDmsEventHolder *pEventHolder,
                                          IDmsSUCacheHolder *pCacheHolder ) ;

         OSS_INLINE virtual UINT32 getMask () const
         {
            return DMS_EVENT_MASK_PLAN ;
         }

         OSS_INLINE virtual const CHAR *getName() const
         {
            return "access plan manager" ;
         }

      protected :
         INT32 _getCLAccessPlan ( const rtnQueryOptions &options,
                                  dmsStorageUnit *su,
                                  dmsMBContext *mbContext,
                                  optAccessPlanRuntime &planRuntime,
                                  const rtnExplainOptions *expOptions ) ;

         INT32 _getCLAccessPlan ( const rtnQueryOptions &options,
                                  OPT_PLAN_CACHE_LEVEL cacheLevel,
                                  dmsStorageUnit *su,
                                  dmsMBContext *mbContext,
                                  optAccessPlanRuntime &planRuntime,
                                  const rtnExplainOptions *expOptions ) ;

         INT32 _getMainCLAccessPlan ( const rtnQueryOptions &options,
                                      dmsStorageUnit *su,
                                      dmsMBContext *mbContext,
                                      optAccessPlanRuntime &planRuntime ) ;

         INT32 _prepareAccessPlanKey ( dmsStorageUnit *su,
                                       dmsMBContext *mbContext,
                                       optAccessPlanKey &planKey,
                                       optAccessPlanHelper &planHelper,
                                       optAccessPlanRuntime &planRuntime ) ;

         INT32 _createAccessPlan ( dmsStorageUnit *su,
                                   dmsMBContext *mbContext,
                                   optAccessPlanKey &planKey,
                                   optAccessPlanRuntime &planRuntime,
                                   optAccessPlanHelper &planHelper,
                                   optGeneralAccessPlan **ppPlan,
                                   BOOLEAN needCache ) ;

         INT32 _getCachedAccessPlan ( const optAccessPlanKey &planKey,
                                      optAccessPlan **ppPlan ) ;

         BOOLEAN _cacheAccessPlan ( optAccessPlan *pPlan ) ;

         // Helpers for parameterized plans
         INT32 _validateParamPlan ( dmsStorageUnit *su,
                                    dmsMBContext *mbContext,
                                    optAccessPlanKey &planKey,
                                    optAccessPlanRuntime &planRuntime,
                                    optAccessPlanHelper &planHelper,
                                    optParamAccessPlan *plan ) ;

         // Helpers for main-collection plans
         INT32 _createMainCLPlan ( optAccessPlanKey &planKey,
                                   const rtnQueryOptions &subOptions,
                                   dmsStorageUnit *su,
                                   dmsMBContext *mbContext,
                                   optAccessPlanRuntime &planRuntime,
                                   optAccessPlanHelper &planHelper,
                                   optMainCLAccessPlan **ppPlan ) ;

         INT32 _validateMainCLPlan ( optMainCLAccessPlan *mainPlan,
                                     const rtnQueryOptions &subOptions,
                                     dmsStorageUnit *su,
                                     dmsMBContext *mbContext,
                                     optAccessPlanRuntime &planRuntime,
                                     optAccessPlanHelper &planHelper ) ;

         INT32 _bindMainCLPlan ( optMainCLAccessPlan *mainPlan,
                                 const rtnQueryOptions &subOptions,
                                 dmsStorageUnit *su,
                                 dmsMBContext *mbContext,
                                 optAccessPlanRuntime &planRuntime,
                                 optAccessPlanHelper &planHelper ) ;

         // Helpers for _IDmsEventHandler
         void _invalidSUPlans ( IDmsSUCacheHolder *pCacheHolder ) ;

         void _invalidCLPlans ( IDmsSUCacheHolder *pCacheHolder,
                                UINT16 mbID, UINT32 clLID ) ;

         void _resetSUPlanCache ( IDmsSUCacheHolder *pCacheHolder ) ;

         // Helpers for clear background job
         INT32 _startClearJob () ;
         void  _stopClearJob () ;

      protected :
         monSpinXLatch           _reinitLatch ;
         optAccessPlanCache      _planCache ;
         optCachedPlanMonitor    _monitor ;
         EDUID                   _clearJobEduID ;

         ossAtomicSigned64       _accessPlanIdGenerator ;

         // Configured options
         OPT_PLAN_CACHE_LEVEL    _cacheLevel ;
   } ;

   typedef class _optAccessPlanManager optAccessPlanManager ;
}

#endif //OPTAPM_HPP__
