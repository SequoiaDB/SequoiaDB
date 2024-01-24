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

   Source File Name = optAPM.cpp

   Descriptive Name = Optimizer Access Plan Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Optimizer Access Plan
   Manager, which is used to pool access plans that previously generated.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft
          01/07/2017  HGM Move from rtnAPM.cpp

   Last Changed =

*******************************************************************************/

#include "optAPM.hpp"
#include "rtn.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsCB.hpp"
#include "pdTrace.hpp"
#include "optTrace.hpp"
#include "pmd.hpp"
#include "optPlanClearJob.hpp"

namespace engine
{

   /*
      _optAccessPlanCache implement
    */
   _optAccessPlanCache::_optAccessPlanCache ()
   : _utilHashTable< optAccessPlanKey, optAccessPlan >(),
     _pMonitor( NULL )
   {
   }

   _optAccessPlanCache::~_optAccessPlanCache ()
   {
      deinitialize() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPCACHES_INIT, "_optAccessPlanCache::initialize" )
   BOOLEAN _optAccessPlanCache::initialize ( UINT32 bucketNum,
                                             optCachedPlanMonitor *pMonitor )
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_OPTAPCACHES_INIT ) ;

      SDB_ASSERT( NULL != pMonitor, "pMonotir is invalid" ) ;

      if ( !utilHashTable::initialize( bucketNum ) )
      {
         goto error ;
      }

      _pMonitor = pMonitor ;
      result = TRUE ;

   done :
      PD_TRACE_EXIT( SDB_OPTAPCACHES_INIT ) ;
      return result ;

   error :
      deinitialize() ;
      result = FALSE ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPCACHES_DEINIT, "_optAccessPlanCache::deinitialize" )
   void _optAccessPlanCache::deinitialize ()
   {
      PD_TRACE_ENTRY( SDB_OPTAPCACHES_DEINIT ) ;

      utilHashTable::deinitialize() ;
      _pMonitor = NULL ;

      PD_TRACE_EXIT( SDB_OPTAPCACHES_DEINIT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPCACHES_ADDPLAN, "_optAccessPlanCache::addPlan" )
   BOOLEAN _optAccessPlanCache::addPlan ( optAccessPlan *pPlan )
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_OPTAPCACHES_ADDPLAN ) ;

      // No need to increase the reference count
      // The new created plan already has 1 reference count
      result = addItem( pPlan ) ;

      PD_TRACE_EXIT( SDB_OPTAPCACHES_ADDPLAN ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPCACHES_RMPLAN, "_optAccessPlanCache::removeCachedPlan" )
   void _optAccessPlanCache::removeCachedPlan ( optAccessPlan *pPlan,
                                                INT32 lockType )
   {
      PD_TRACE_ENTRY( SDB_OPTAPCACHES_RMPLAN ) ;

      if ( SHARED == lockType )
      {
         _pMonitor->getClearLock()->lock_r() ;
      }
      else if ( EXCLUSIVE == lockType )
      {
         _pMonitor->getClearLock()->lock_w() ;
      }

      // Increase the reference count before we delete it
      pPlan->incRefCount() ;
      if ( removeItem( pPlan ) )
      {
         // We need to reset the activity ID to check if
         // someone else is also deleting this plan
         _pMonitor->resetActivity( pPlan->resetActivityID() ) ;
      }
      pPlan->release() ;

      if ( SHARED == lockType )
      {
         _pMonitor->getClearLock()->release_r() ;
      }
      else if ( EXCLUSIVE == lockType )
      {
         _pMonitor->getClearLock()->release_w() ;
      }

      PD_TRACE_EXIT( SDB_OPTAPCACHES_RMPLAN ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPCACHES_RSPLANACT, "_optAccessPlanCache::resetCachedPlanActivity" )
   void _optAccessPlanCache::resetCachedPlanActivity( optAccessPlan *pPlan,
                                                      INT32 lockType )
   {
      PD_TRACE_ENTRY( SDB_OPTAPCACHES_RSPLANACT ) ;

      if ( SHARED == lockType )
      {
         _pMonitor->getClearLock()->lock_r() ;
      }
      else if ( EXCLUSIVE == lockType )
      {
         _pMonitor->getClearLock()->lock_w() ;
      }

      // We need to reset the activity ID to check if
      // someone else is also deleting this plan
      _pMonitor->resetActivity( pPlan->resetActivityID() ) ;

      if ( SHARED == lockType )
      {
         _pMonitor->getClearLock()->release_r() ;
      }
      else if ( EXCLUSIVE == lockType )
      {
         _pMonitor->getClearLock()->release_w() ;
      }

      PD_TRACE_EXIT( SDB_OPTAPCACHES_RSPLANACT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPCACHES_INVALIDSUPLANS, "_optAccessPlanCache::invalidateSUPlans" )
   void _optAccessPlanCache::invalidateSUPlans ( dmsCachedPlanMgr *pCachedPlanMgr,
                                                 UINT32 suLID )
   {
      PD_TRACE_ENTRY( SDB_OPTAPCACHES_INVALIDSUPLANS ) ;

      SDB_ASSERT( pCachedPlanMgr, "pCachedPlanMgr is invalid" ) ;

      ossScopedRWLock scopedLock( &_bucketNumLock, SHARED ) ;

      for ( UINT32 bucketID = 0 ; bucketID < getBucketNum() ; bucketID ++ )
      {
         if ( pCachedPlanMgr->getBucketNum() != getBucketNum() ||
              pCachedPlanMgr->testCacheBitmap( bucketID ) )
         {
            // Lock the clear lock shared, parallel removing for different
            // collections or collection spaces is allowed
            ossScopedRWLock scopedLock( _pMonitor->getClearLock(), SHARED ) ;
            utilHashTableBucket *pBucket = getBucket( bucketID, EXCLUSIVE ) ;

            if ( NULL != pBucket )
            {
               optAccessPlan *pPlan = pBucket->getHead() ;
               while ( NULL != pPlan )
               {
                  optAccessPlan *pNextPlan = (optAccessPlan *)pPlan->getNext() ;
                  if ( pPlan->getSULID() == suLID )
                  {
                     // Locked bucket already, safe to remove from bucket
                     if ( pBucket->removeItem( pPlan ) )
                     {
                        // We need to reset the activity ID to check if someone
                        // else is also deleting this plan
                        _pMonitor->resetActivity( pPlan->resetActivityID() ) ;
                     }

                     pPlan->release() ;
                  }
                  pPlan = pNextPlan ;
               }
               releaseBucket( bucketID, EXCLUSIVE ) ;
            }
            else
            {
               SDB_ASSERT( pBucket, "pBucket is invalid" ) ;
            }
         }
      }

      PD_TRACE_EXIT( SDB_OPTAPCACHES_INVALIDSUPLANS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPCACHES_INVALIDCLPLANS, "_optAccessPlanCache::invalidateCLPlans" )
   void _optAccessPlanCache::invalidateCLPlans ( dmsCachedPlanMgr *pCachedPlanMgr,
                                                 UINT32 suLID, UINT32 clLID )
   {
      PD_TRACE_ENTRY( SDB_OPTAPCACHES_INVALIDCLPLANS ) ;

      SDB_ASSERT( pCachedPlanMgr, "pCachedPlanMgr is invalid" ) ;

      ossScopedRWLock scopedLock( &_bucketNumLock, SHARED ) ;

      for ( UINT32 bucketID = 0 ; bucketID < getBucketNum() ; bucketID ++ )
      {
         if ( pCachedPlanMgr->getBucketNum() != getBucketNum() ||
              pCachedPlanMgr->testCacheBitmap( bucketID ) )
         {
            // Lock the clear lock shared, parallel removing for different
            // collections or collection spaces is allowed
            ossScopedRWLock scopedLock( _pMonitor->getClearLock(), SHARED ) ;
            utilHashTableBucket *pBucket = getBucket( bucketID, EXCLUSIVE ) ;
            BOOLEAN clearBit = TRUE ;

            if ( NULL != pBucket )
            {
               optAccessPlan *pPlan = pBucket->getHead() ;
               while ( pPlan )
               {
                  optAccessPlan *pNextPlan = (optAccessPlan *)pPlan->getNext() ;
                  if ( pPlan->getSULID() == suLID && pPlan->getCLLID() == clLID )
                  {
                     // Locked bucket already, safe to remove from bucket
                     if ( pBucket->removeItem( pPlan ) )
                     {
                        // We need to reset the activity ID to check if someone
                        // else is also deleting this plan
                        _pMonitor->resetActivity( pPlan->resetActivityID() ) ;
                     }
                     else if ( clearBit )
                     {
                        // The plan is not removed, so to be safe,
                        // could not clear the bit
                        clearBit = FALSE ;
                     }

                     pPlan->release() ;
                  }
                  else if ( clearBit && pPlan->getSULID() == suLID )
                  {
                     // Still contains plans from the same collection space
                     // could not clear the bit
                     clearBit = FALSE ;
                  }
                  pPlan = pNextPlan ;
               }

               if ( clearBit )
               {
                  // Bucket contains no plans of this SU any more
                  // clear the bit
                  pCachedPlanMgr->clearCacheBit( bucketID ) ;
               }

               releaseBucket( bucketID, EXCLUSIVE ) ;
            }
            else
            {
               SDB_ASSERT( pBucket, "pBucket is invalid" ) ;
            }
         }
      }

      PD_TRACE_EXIT( SDB_OPTAPCACHES_INVALIDCLPLANS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPCACHES_INVALIDALLPLANS, "_optAccessPlanCache::invalidateAllPlans" )
   void _optAccessPlanCache::invalidateAllPlans ()
   {
      PD_TRACE_ENTRY( SDB_OPTAPCACHES_INVALIDALLPLANS ) ;

      ossScopedRWLock scopedLock( &_bucketNumLock, SHARED ) ;

      for ( UINT32 bucketID = 0 ; bucketID < getBucketNum() ; bucketID ++ )
      {
         ossScopedRWLock scopedLock( _pMonitor->getClearLock(), EXCLUSIVE ) ;
         utilHashTableBucket *pBucket = getBucket( bucketID, EXCLUSIVE ) ;

         if ( NULL != pBucket )
         {
            optAccessPlan *pPlan = pBucket->getHead() ;
            while ( pPlan )
            {
               optAccessPlan *pNextPlan = (optAccessPlan *)pPlan->getNext() ;

               // Locked bucket already, safe to remove from bucket
               if ( pBucket->removeItem( pPlan ) )
               {
                  // We need to reset the activity ID to check if someone
                  // else is also deleting this plan
                  _pMonitor->resetActivity( pPlan->resetActivityID() ) ;
               }
               pPlan->release() ;
               pPlan = pNextPlan ;
            }

            releaseBucket( bucketID, EXCLUSIVE ) ;
         }
         else
         {
            SDB_ASSERT( pBucket, "pBucket is invalid" ) ;
         }
      }

      PD_TRACE_EXIT( SDB_OPTAPCACHES_INVALIDALLPLANS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPCACHES_INVALIDCLPLANS_NAME, "_optAccessPlanCache::invalidateCLPlans" )
   void _optAccessPlanCache::invalidateCLPlans ( const CHAR *pCLFullName )
   {
      PD_TRACE_ENTRY( SDB_OPTAPCACHES_INVALIDCLPLANS_NAME ) ;

      SDB_ASSERT( NULL != pCLFullName, "collection name is invalid" ) ;

      ossScopedRWLock scopedLock( &_bucketNumLock, SHARED ) ;

      for ( UINT32 bucketID = 0 ; bucketID < getBucketNum() ; bucketID ++ )
      {
         // Lock the clear lock shared, parallel removing for different
         ossScopedRWLock scopedLock( _pMonitor->getClearLock(), SHARED ) ;
         utilHashTableBucket *pBucket = getBucket( bucketID, EXCLUSIVE ) ;

         if ( NULL != pBucket )
         {
            optAccessPlan *pPlan = pBucket->getHead() ;
            while ( pPlan )
            {
               optAccessPlan *pNextPlan = (optAccessPlan *)pPlan->getNext() ;
               if ( 0 == ossStrncmp( pCLFullName, pPlan->getCLFullName(),
                                     DMS_COLLECTION_FULL_NAME_SZ ) )
               {
                  // Locked bucket already, safe to remove from bucket
                  if ( pBucket->removeItem( pPlan ) )
                  {
                     // We need to reset the activity ID to check if someone
                     // else is also deleting this plan
                     _pMonitor->resetActivity( pPlan->resetActivityID() ) ;
                  }
                  pPlan->release() ;
               }
               pPlan = pNextPlan ;
            }

            releaseBucket( bucketID, EXCLUSIVE ) ;
         }
         else
         {
            SDB_ASSERT( pBucket, "pBucket is invalid" ) ;
         }
      }

      PD_TRACE_EXIT( SDB_OPTAPCACHES_INVALIDCLPLANS_NAME ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPCACHES_INVALIDSUPLANS_NAME, "_optAccessPlanCache::invalidateSUPlans" )
   void _optAccessPlanCache::invalidateSUPlans ( const CHAR * pCSName )
   {
      PD_TRACE_ENTRY( SDB_OPTAPCACHES_INVALIDSUPLANS_NAME ) ;

      SDB_ASSERT( NULL != pCSName, "collection space name is invalid" ) ;

      ossScopedRWLock scopedLock( &_bucketNumLock, SHARED ) ;

      for ( UINT32 bucketID = 0 ; bucketID < getBucketNum() ; bucketID ++ )
      {
         // Lock the clear lock shared, parallel removing for different
         ossScopedRWLock scopedLock( _pMonitor->getClearLock(), SHARED ) ;
         utilHashTableBucket *pBucket = getBucket( bucketID, EXCLUSIVE ) ;

         if ( NULL != pBucket )
         {
            optAccessPlan *pPlan = pBucket->getHead() ;
            while ( pPlan )
            {
               INT32 rc = SDB_OK ;
               CHAR csName [ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;

               optAccessPlan *pNextPlan = (optAccessPlan *)pPlan->getNext() ;

               rc = rtnResolveCollectionSpaceName(
                     pPlan->getCLFullName(), ossStrlen( pPlan->getCLFullName() ),
                     csName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDDEBUG, "Failed to resolve collection space "
                          "name [%s], rc: %d", pPlan->getCLFullName(), rc ) ;
               }

               if ( SDB_OK != rc ||
                    0 == ossStrncmp( pCSName, csName,
                                     DMS_COLLECTION_SPACE_NAME_SZ ) )
               {
                  // Locked bucket already, safe to remove from bucket
                  if ( pBucket->removeItem( pPlan ) )
                  {
                     // We need to reset the activity ID to check if someone
                     // else is also deleting this plan
                     _pMonitor->resetActivity( pPlan->resetActivityID() ) ;
                  }
                  pPlan->release() ;
               }
               pPlan = pNextPlan ;
            }

            releaseBucket( bucketID, EXCLUSIVE ) ;
         }
         else
         {
            SDB_ASSERT( pBucket, "pBucket is invalid" ) ;
         }
      }

      PD_TRACE_EXIT( SDB_OPTAPCACHES_INVALIDSUPLANS_NAME ) ;
   }

   UINT32 _optAccessPlanCache::getCachedPlanCount () const
   {
      return _pMonitor->getCachedPlanCount() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCPCACHE_GETCPLIST, "_optAccessPlanCache::getCachedPlanList" )
   INT32 _optAccessPlanCache::getCachedPlanList ( vector<BSONObj> &cachedPlanList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTCPCACHE_GETCPLIST ) ;

      ossScopedRWLock scopedLock( &_bucketNumLock, SHARED ) ;

      for ( UINT32 bucketID = 0 ; bucketID < getBucketNum() ; bucketID ++ )
      {
         utilHashTableBucket *pBucket = getBucket( bucketID, SHARED ) ;

         SDB_ASSERT( pBucket, "pBucket is invalid" ) ;

         optAccessPlan *pPlan = pBucket->getHead() ;
         while ( pPlan )
         {
            try
            {
               BSONObjBuilder planBuilder ;
               BSONObj planObj ;

               const CHAR *clFullName = pPlan->getCLFullName() ;
               CHAR csName [ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;

               rc = rtnResolveCollectionSpaceName( clFullName,
                                                   ossStrlen( clFullName ),
                                                   csName,
                                                   DMS_COLLECTION_SPACE_NAME_SZ ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to resolve collection space "
                          "name, rc: %d", rc ) ;
                  // continue to next
                  rc = SDB_OK ;
                  pPlan = (optAccessPlan *)pPlan->getNext() ;
                  continue ;
               }

               planBuilder.append( OPT_FIELD_ACCESSPLAN_ID,
                                   pPlan->getAccessPlanID() ) ;
               planBuilder.append( OPT_FIELD_COLLECTION,
                                   pPlan->getCLFullName() ) ;
               planBuilder.append( OPT_FIELD_COLLECTION_SPACE, csName ) ;
               planBuilder.append( OPT_FIELD_SCAN_TYPE,
                                   IXSCAN == pPlan->getScanType() ?
                                   OPT_VALUE_IXSCAN : OPT_VALUE_TBSCAN ) ;
               planBuilder.append( OPT_FIELD_INDEX_NAME,
                                   pPlan->getIndexName() ) ;
               planBuilder.appendBool( OPT_FIELD_USE_EXT_SORT,
                                       pPlan->sortRequired() ) ;

               // no need to set abbrev with explain options, the plan with
               // large size string will not be cached
               pPlan->toBSON( planBuilder ) ;

               if ( NULL != _pMonitor )
               {
                  optCachedPlanActivity *activity =
                              _pMonitor->getActivity( pPlan->getActivityID() ) ;
                  if ( NULL != activity )
                  {
                     activity->toBSON( planBuilder ) ;
                  }
               }

               planObj = planBuilder.obj() ;
               cachedPlanList.push_back( planObj ) ;
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDERROR, "Failed to build result, received "
                       "unexpected error: %s", e.what() ) ;
               // continue to next
            }

            pPlan = (optAccessPlan *)pPlan->getNext() ;
         }

         releaseBucket( bucketID, SHARED ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTCPCACHE_GETCPLIST, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCPCACHE_ENABLECACHE, "_optAccessPlanCache::enableCaching" )
   void _optAccessPlanCache::enableCaching ()
   {
      PD_TRACE_ENTRY( SDB_OPTCPCACHE_ENABLECACHE ) ;
      if ( isInitialized() )
      {
         _setEnableAddItem( TRUE ) ;
      }
      PD_TRACE_EXIT( SDB_OPTCPCACHE_ENABLECACHE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCPCACHE_DISABLECACHE, "_optAccessPlanCache::disableCaching" )
   void _optAccessPlanCache::disableCaching ()
   {
      PD_TRACE_ENTRY( SDB_OPTCPCACHE_DISABLECACHE ) ;
      if ( isInitialized() )
      {
         _setEnableAddItem( FALSE ) ;
      }
      PD_TRACE_EXIT( SDB_OPTCPCACHE_DISABLECACHE ) ;
   }

   void _optAccessPlanCache::_afterAddItem ( UINT32 bucketID,
                                             optAccessPlan *pPlan )
   {
      SDB_ASSERT( pPlan, "pPlan is invalid" ) ;
      pPlan->setCachedBitmap() ;
      pPlan->incRefCount() ;
   }

   void _optAccessPlanCache::_afterGetItem ( UINT32 bucketID,
                                             optAccessPlan *pPlan )
   {
      SDB_ASSERT( pPlan, "pPlan is invalid" ) ;

      pPlan->incRefCount() ;
      _pMonitor->setCachedPlanActivity( pPlan ) ;
   }

   void _optAccessPlanCache::_afterRemoveItem( UINT32 bucketID,
                                               optAccessPlan *pPlan )
   {
      SDB_ASSERT( pPlan, "pPlan is invalid" ) ;
      pPlan->decRefCount() ;
   }

   /*
      _optCachedPlanActivity implement
    */
   _optCachedPlanActivity::_optCachedPlanActivity ()
   : _pPlan( NULL ),
     _lastAccessTime( 0 ),
     _periodAccessCount( 0 ),
     _accessCount( 0 ),
     _totalQueryTimeTick(),
     _latch( MON_LATCH_OPTCACHEDPLANACTIVITY_LATCH )
   {
   }

   _optCachedPlanActivity::~_optCachedPlanActivity ()
   {
      _clearPlan() ;
   }

   void _optCachedPlanActivity::clear ()
   {
      // Lock the mutex to exclude setting query activities
      ossScopedLock lock( &_latch ) ;

      _lastAccessTime = 0 ;
      _periodAccessCount = 0 ;
      _accessCount = 0 ;
      _totalQueryTimeTick.clear() ;
      _maxQueryActivity.reset() ;
      _minQueryActivity.reset() ;
      _clearPlan() ;
   }

   void _optCachedPlanActivity::setPlan ( optAccessPlan *pPlan,
                                          UINT64 timestamp )
   {
      // Lock the mutex to exclude setting query activities
      ossScopedLock lock( &_latch ) ;

      _lastAccessTime = timestamp ;
      _setPlan( pPlan ) ;
      _periodAccessCount = 0 ;
      _accessCount = 0 ;
      _totalQueryTimeTick.clear() ;
      _maxQueryActivity.reset() ;
      _minQueryActivity.reset() ;
   }

   void _optCachedPlanActivity::setQueryActivity (
                                    const optQueryActivity &queryActivity,
                                    const rtnParamList &parameters )
   {
      if ( !isEmpty() )
      {
         /// The function is called by concurrent, so need to mutex
         /// each other
         ossScopedLock lock( &_latch ) ;

         // Re-check if already removed from cached
         if ( isEmpty() ) {
            return ;
         }

         _totalQueryTimeTick += queryActivity.getQueryTime() ;
         if ( queryActivity.getQueryTime() < _minQueryActivity.getQueryTime() ||
              !_minQueryActivity.isValid() )
         {
            _minQueryActivity = queryActivity ;
            _minQueryActivity.setParameters( parameters ) ;
         }
         if ( queryActivity.getQueryTime() > _maxQueryActivity.getQueryTime() ||
              !_maxQueryActivity.isValid() )
         {
            _maxQueryActivity = queryActivity ;
            _maxQueryActivity.setParameters( parameters ) ;
         }
         incAccessCount() ;
      }
   }

   void _optCachedPlanActivity::toBSON ( BSONObjBuilder &builder )
   {
      ossTickConversionFactor factor ;
      UINT32 seconds = 0, microseconds = 0 ;
      double totalQueryTime = 0.0, avgQueryTime = 0.0 ;
      UINT64 accessCount = _accessCount ;

      _totalQueryTimeTick.convertToTime( factor, seconds, microseconds ) ;
      totalQueryTime = (double)( seconds ) +
                       (double)( microseconds ) / (double)( OSS_ONE_MILLION ) ;
      if ( accessCount > 0 )
      {
         avgQueryTime = totalQueryTime / (double)accessCount ;
      }

      builder.append( OPT_FIELD_PLAN_ACCESS_COUNT, (INT64)( accessCount ) ) ;

      builder.append( OPT_FIELD_TOTAL_QUERY_TIME, totalQueryTime ) ;
      builder.append( OPT_FIELD_AVG_QUERY_TIME, avgQueryTime ) ;

      {
         /// When toBSON, the setQueryActivity will be called con-currency.
         /// So, need to mutex
         ossScopedLock lock( &_latch ) ;

         if ( _maxQueryActivity.isValid() )
         {
            BSONObjBuilder maxBuilder( builder.subobjStart( OPT_FIELD_MAX_QUERY ) ) ;
            _maxQueryActivity.toBSON( maxBuilder ) ;
            maxBuilder.done() ;
         }
         if ( _minQueryActivity.isValid() )
         {
            BSONObjBuilder minBuilder( builder.subobjStart( OPT_FIELD_MIN_QUERY ) ) ;
            _minQueryActivity.toBSON( minBuilder ) ;
            minBuilder.done() ;
         }
      }
   }

   /*
      _optCachedPlanMonitor implement
    */
   _optCachedPlanMonitor::_optCachedPlanMonitor ()
   : _freeIndexBegin( 0 ),
     _freeIndexEnd( 0 ),
     _pFreeActivityIDs( NULL ),
     _clearThread( 0 ),
     _clearLock( MON_LATCH_OPTCACHEDPLANMONITOR_CLEARLOCK ),
     _activityNum( 0 ),
     _highWaterMark( 0 ),
     _lowWaterMark( 0 ),
     _clockIndex( 0 ),
     _pActivities( NULL ),
     _cachedPlanCount( 0 ),
     _accessTimestamp( 0 ),
     _lastClearTimestamp( 0 ),
     _pPlanCache( NULL )
   {
   }

   _optCachedPlanMonitor::~_optCachedPlanMonitor ()
   {
      deinitialize() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCPMON_INIT, "_optCachedPlanMonitor::initialize" )
   BOOLEAN _optCachedPlanMonitor::initialize ( optAccessPlanCache *pPlanCache )
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_OPTCPMON_INIT ) ;

      UINT32 activityNum = 0 ;

      if ( isInitialized() )
      {
         result = TRUE ;
         goto done ;
      }

      if ( NULL != pPlanCache && !pPlanCache->isInitialized() )
      {
         goto error ;
      }

      activityNum = pPlanCache->getBucketNum() *
                    OPT_PLAN_CACHE_AVG_BUCKET_SIZE ;
      if ( activityNum == 0 )
      {
         goto error ;
      }

      // Allocate activity buffer
      _pFreeActivityIDs = ( UINT32* ) SDB_OSS_MALLOC( activityNum *
                                                      sizeof( UINT32 ) ) ;
      if ( NULL == _pFreeActivityIDs )
      {
         goto error ;
      }

      _pActivities = SDB_OSS_NEW optCachedPlanActivity[ activityNum ] ;
      if ( NULL == _pActivities )
      {
         goto error ;
      }

      _activityNum = activityNum ;
      _highWaterMark = activityNum * OPT_PLAN_CACHE_ACT_HIGH_PERC ;
      _lowWaterMark = activityNum * OPT_PLAN_CACHE_ACT_LOW_PERC ;
      _pPlanCache = pPlanCache ;

      for ( UINT32 i = 0 ; i < activityNum ; i++ )
      {
         _pFreeActivityIDs[ i ] = i ;
      }

      _freeIndexBegin.init( 0 ) ;
      _freeIndexEnd.init( _activityNum ) ;
      _clearThread.init( 0 ) ;
      _clockIndex = 0 ;

      _cachedPlanCount.init( 0 ) ;
      _accessTimestamp.init( 0 ) ;

      _lastClearTimestamp = 0 ;

      result = TRUE ;

   done :
      PD_TRACE_EXIT( SDB_OPTCPMON_INIT ) ;
      return result ;

   error :
      deinitialize() ;
      result = FALSE ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCPMON_DEINIT, "_optCachedPlanMonitor::deinitialize" )
   void _optCachedPlanMonitor::deinitialize ()
   {
      PD_TRACE_ENTRY( SDB_OPTCPMON_DEINIT ) ;

      if ( NULL != _pFreeActivityIDs )
      {
         SDB_OSS_FREE( _pFreeActivityIDs ) ;
      }
      if ( NULL != _pActivities )
      {
         SDB_OSS_DEL [] _pActivities ;
      }
      _pFreeActivityIDs = NULL ;
      _pActivities = NULL ;
      _activityNum = 0 ;
      _highWaterMark = 0 ;
      _lowWaterMark = 0 ;
      _freeIndexBegin.init( 0 ) ;
      _freeIndexEnd.init( 0 ) ;
      _clearThread.init( 0 ) ;
      _clockIndex = 0 ;
      _pPlanCache = NULL ;

      _cachedPlanCount.init( 0 ) ;
      _accessTimestamp.init( 0 ) ;

      _lastClearTimestamp = 0 ;

      PD_TRACE_EXIT( SDB_OPTCPMON_DEINIT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCPMON_SETACT, "_optCachedPlanMonitor::setActivity" )
   BOOLEAN _optCachedPlanMonitor::setActivity ( optAccessPlan *pPlan )
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_OPTCPMON_SETACT ) ;

      INT32 activityID = OPT_INVALID_ACT_ID ;

      if ( _cachedPlanCount.inc() > _highWaterMark )
      {
         signalPlanClearJob() ;
      }

      activityID = _allocateActivity( pPlan ) ;
      if ( OPT_INVALID_ACT_ID == activityID )
      {
         _cachedPlanCount.dec() ;
      }
      else
      {
         optCachedPlanActivity &activity = _pActivities[ activityID ] ;

         SDB_ASSERT( activity.isEmpty(), "Activity is not empty" ) ;

         activity.setPlan( pPlan, _accessTimestamp.inc() ) ;
         activity.incPeriodAccessCount() ;
         pPlan->setActivityID( activityID ) ;

         result = TRUE ;
      }

      PD_TRACE_EXIT( SDB_OPTCPMON_SETACT ) ;
      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCPMON_SIGNALJOB, "_optCachedPlanMonitor::signalPlanClearJob" )
   void _optCachedPlanMonitor::signalPlanClearJob ()
   {
      PD_TRACE_ENTRY( SDB_OPTCPMON_SIGNALJOB ) ;
      _clearEvent.signal() ;
      PD_TRACE_EXIT( SDB_OPTCPMON_SIGNALJOB ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCPMON_CLEARCP, "_optCachedPlanMonitor::clearCachedPlans" )
   void _optCachedPlanMonitor::clearCachedPlans ()
   {
      PD_TRACE_ENTRY( SDB_OPTCPMON_CLEARCP ) ;

      if ( _clearThread.compareAndSwap( 0, 1 ) )
      {
         // Only one thread could enter this branch

         // Lock the clear lock exclusively, so other threads could not
         // remove plans during this procedure
         ossScopedRWLock scopedLock( &_clearLock, EXCLUSIVE ) ;

         UINT32 needRemoveCount = 0 ;
         double avgClearScore = 0.0 ;
         UINT64 currentTimestamp = 0 ;
         UINT64 totalAccessCount = 0 ;
         UINT64 avgAccessCount = 0 ;
         UINT32 lastClockIndex = _clockIndex ;

         // Check again after lock
         UINT32 cachedPlanCount = _cachedPlanCount.fetch() ;
         if ( cachedPlanCount < _highWaterMark )
         {
            _clearThread.swap( 0 ) ;
            goto done ;
         }

         needRemoveCount = cachedPlanCount - _lowWaterMark ;

         PD_LOG( PDDEBUG, "Cached Plan Monitor: %u plans are cached, "
                 "%u need to be removed", cachedPlanCount, needRemoveCount ) ;

         // Calculate the average clear score of all cached plans
         currentTimestamp = _accessTimestamp.inc() ;

         // Total access count is difference between current timestamp and last
         // clear timestamp since the logical timestamp is used
         totalAccessCount = currentTimestamp - _lastClearTimestamp ;
         totalAccessCount = OSS_MAX( 1, totalAccessCount ) ;

         avgAccessCount = totalAccessCount / cachedPlanCount ;
         avgAccessCount = OSS_MAX( 1, avgAccessCount ) ;

         avgClearScore = 1.0 / (double)cachedPlanCount ;

         // End searching conditions:
         // 1. removed enough plans
         // 2. searched one loop
         while ( needRemoveCount > 0 )
         {
            optCachedPlanActivity &activity = _pActivities[ _clockIndex ] ;
            UINT64 accessTime = 0 ;
            UINT64 accessCount = 0 ;
            double curClearScore = 0.0 ;

            if ( activity.isEmpty() )
            {
               _clockIndex = ( _clockIndex + 1 ) % _activityNum ;
               // Searched one loop
               if ( _clockIndex == lastClockIndex )
               {
                  break ;
               }
               continue ;
            }

            // Calculate the clear score of current plan
            accessTime = activity.getLastAccessTime() ;
            accessCount = activity.getPeriodAccessCount() ;
            curClearScore = (double)accessCount /
                            (double)( currentTimestamp - accessTime ) ;

            if ( curClearScore > -OSS_EPSILON &&
                 curClearScore < avgClearScore )
            {
               // The score is smaller than average score, clear the plan
               // NOTE: the score < 0.0, means access time is larger than
               // current clear time
               _pPlanCache->removeCachedPlan( activity.getPlan() ) ;
               needRemoveCount -- ;
            }
            else
            {
               // Decrease the access count by average access count
               activity.decPeriodAccessCount( avgAccessCount ) ;
            }

            _clockIndex = ( _clockIndex + 1 ) % _activityNum ;
            if ( _clockIndex == lastClockIndex )
            {
               // Searched one loop and could be stopped
               break ;
            }
         }

         PD_LOG( PDDEBUG, "Cached Plan Monitor: cleared %u cached plans, "
                 "%u left", cachedPlanCount - _lowWaterMark - needRemoveCount,
                 _cachedPlanCount.peek() ) ;

         _lastClearTimestamp = _accessTimestamp.inc() ;
         _clearThread.swap( 0 ) ;
      }

   done :
      PD_TRACE_EXIT( SDB_OPTCPMON_CLEARCP ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCPMON_CLEAREXPCP, "_optCachedPlanMonitor::clearExpiredCachedPlans" )
   void _optCachedPlanMonitor::clearExpiredCachedPlans ()
   {
      PD_TRACE_ENTRY( SDB_OPTCPMON_CLEAREXPCP ) ;

      if ( _clearThread.compareAndSwap( 0, 1 ) )
      {
         // Only one thread could enter this branch

         UINT32 removedCount = 0 ;

         // Lock the clear lock exclusively, so other threads could not
         // remove plans during this procedure
         ossScopedRWLock scopedLock( &_clearLock, EXCLUSIVE ) ;

         UINT64 expiredTimestamp = _lastClearTimestamp ;
         UINT32 lastClockIndex = _clockIndex ;

         while ( TRUE )
         {
            optCachedPlanActivity &activity = _pActivities[ _clockIndex ] ;
            if ( activity.isEmpty() )
            {
               _clockIndex = ( _clockIndex + 1 ) % _activityNum ;
               // Searched one loop
               if ( _clockIndex == lastClockIndex )
               {
                  break ;
               }
               continue ;
            }

            // plan is not accessed after expired timestamp, clear it
            if ( activity.getLastAccessTime() < expiredTimestamp )
            {
               _pPlanCache->removeCachedPlan( activity.getPlan() ) ;
               ++ removedCount ;
            }

            _clockIndex = ( _clockIndex + 1 ) % _activityNum ;
            if ( _clockIndex == lastClockIndex )
            {
               // Searched one loop and could be stopped
               break ;
            }
         }

         PD_LOG( PDDEBUG, "Cached Plan Monitor: "
                 "cleared %u expired cached plans, %u left", removedCount,
                 _cachedPlanCount.peek() ) ;

         _lastClearTimestamp = _accessTimestamp.inc() ;
         _clearThread.swap( 0 ) ;
      }

      PD_TRACE_EXIT( SDB_OPTCPMON_CLEAREXPCP ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCPMON__ALLOCACT, "_optCachedPlanMonitor::_allocateActivity" )
   INT32 _optCachedPlanMonitor::_allocateActivity ( optAccessPlan *pPlan )
   {
      INT32 activityID = OPT_INVALID_ACT_ID ;

      PD_TRACE_ENTRY( SDB_OPTCPMON__ALLOCACT ) ;

      if ( _cachedPlanCount.fetch() < _activityNum )
      {
         // Get free activity from free index
         UINT64 freeActivityIndex = _freeIndexBegin.inc() ;

#ifdef _DEBUG
         // Do check in debug mode
         UINT64 tmpEndIndex = _freeIndexEnd.fetch() ;
         SDB_ASSERT( freeActivityIndex < tmpEndIndex,
                     "AcitvityIndex must < tmpEndIndex" ) ;
#endif

         activityID = _pFreeActivityIDs[ freeActivityIndex % _activityNum ] ;
      }

      PD_TRACE_EXIT( SDB_OPTCPMON__ALLOCACT ) ;
      return activityID ;
   }

   /*
      _optAccessPlanManager implement
    */
   _optAccessPlanManager::_optAccessPlanManager ()
   : _optAccessPlanConfigHolder(),
     _mthMatchConfigHolder(),
     _reinitLatch( MON_LATCH_OPTACCESSPLANMANAGER_REINITLATCH ),
     _planCache(),
     _monitor(),
     _clearJobEduID( PMD_INVALID_EDUID ),
     _accessPlanIdGenerator( 0 ),
     _cacheLevel( OPT_PLAN_NOCACHE )
   {
   }

   _optAccessPlanManager::~_optAccessPlanManager ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_INIT, "_optAccessPlanManager::init" )
   INT32 _optAccessPlanManager::init ( UINT32 bucketNum,
                                       OPT_PLAN_CACHE_LEVEL cacheLevel,
                                       UINT32 sortBufferSize,
                                       INT32 optCostThreshold,
                                       BOOLEAN enableMixCmp,
                                       INT32 planCacheMainCLThreshold )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_INIT ) ;

      SDB_ASSERT( !_planCache.isInitialized(),
                  "cache should not be initialized" ) ;

      _cacheLevel = OPT_PLAN_NOCACHE ;

      setSortBufferSize( sortBufferSize ) ;
      setOptCostThreshold( optCostThreshold ) ;
      setPlanCacheMainCLThreshold( planCacheMainCLThreshold ) ;

      // Always update mix-compare mode
      setMthEnableMixCmp( enableMixCmp ) ;

      if ( bucketNum > 0 && cacheLevel > OPT_PLAN_NOCACHE )
      {
         bucketNum = _planCache.getRoundedUpBucketNum( bucketNum ) ;

         _planCache.initialize( bucketNum, &_monitor ) ;
         PD_CHECK( _planCache.isInitialized(), SDB_OOM, error, PDERROR,
                   "Failed to initialize plan caches" ) ;

         _monitor.initialize( &_planCache ) ;
         PD_CHECK( _monitor.isInitialized(), SDB_OOM, error, PDERROR,
                   "Failed to initialize plan cache sweeper" ) ;

         // Start cached-plan clearing background job
         rc = _startClearJob() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start cached-plan clearing job "
                      "failed, rc: %d", rc ) ;
      }
      else
      {
         bucketNum = 0 ;
         cacheLevel = OPT_PLAN_NOCACHE ;
      }

      // Change caches in DMS level
      sdbGetDMSCB()->changeSUCaches( getMask() ) ;

      // Update parameterize and fuzzy-operator by cache level
      setMthEnableParameterized( cacheLevel >= OPT_PLAN_PARAMETERIZED ) ;
      setMthEnableFuzzyOptr( cacheLevel >= OPT_PLAN_FUZZYOPTR ) ;

      // Set cache level
      _cacheLevel = cacheLevel ;

      // Done initialize, enable caching
      _planCache.enableCaching() ;

      PD_LOG( PDDEBUG, "Initialize plan cache: [ level: %s, buckets : %u ]",
              optAccessPlanKey::getCacheLevelName( _cacheLevel ),
              _planCache.getBucketNum() ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTAPM_INIT, rc ) ;
      return rc ;

   error :
      fini() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_REINIT, "_optAccessPlanManager::reinit" )
   INT32 _optAccessPlanManager::reinit ( UINT32 bucketNum,
                                         OPT_PLAN_CACHE_LEVEL cacheLevel,
                                         UINT32 sortBufferSize,
                                         INT32 optCostThreshold,
                                         BOOLEAN enableMixCmp,
                                         INT32 planCacheMainCLThreshold )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_REINIT ) ;

      // Only one thread could enter reinitialize process
      ossScopedLock scopedLock( &_reinitLatch ) ;

      if ( _planCache.isInitialized() )
      {
         bucketNum = _planCache.getRoundedUpBucketNum( bucketNum ) ;

         if ( bucketNum != _planCache.getBucketNum() ||
              cacheLevel != _cacheLevel ||
              sortBufferSize != getSortBufferSizeMB() ||
              optCostThreshold != getOptCostThreshold() ||
              enableMixCmp != mthEnabledMixCmp() ||
              planCacheMainCLThreshold != getPlanCacheMainCLThreshold() )
         {
            if ( 0 == bucketNum ||
                 OPT_PLAN_NOCACHE == cacheLevel )
            {
               cacheLevel = OPT_PLAN_NOCACHE ;
               bucketNum = 0 ;
            }

            if ( bucketNum == _planCache.getBucketNum() )
            {
               _planCache.disableCaching() ;

               setSortBufferSize( sortBufferSize ) ;
               setOptCostThreshold( optCostThreshold ) ;
               setMthEnableMixCmp( enableMixCmp ) ;
               setPlanCacheMainCLThreshold( planCacheMainCLThreshold ) ;

               sdbGetDMSCB()->clearSUCaches( DMS_EVENT_MASK_PLAN ) ;

               setMthEnableParameterized( cacheLevel >= OPT_PLAN_PARAMETERIZED ) ;
               setMthEnableFuzzyOptr( cacheLevel >= OPT_PLAN_FUZZYOPTR ) ;
               _cacheLevel = cacheLevel ;

               _planCache.enableCaching() ;

               PD_LOG( PDDEBUG, "Re-initialize plan cache: [ level: %s ]",
                       optAccessPlanKey::getCacheLevelName( _cacheLevel ) ) ;
            }
            else
            {
               // We don't need the clearing job anymore
               if ( 0 == bucketNum )
               {
                  _stopClearJob() ;
               }

               // Deinitialize the cache first
               rc = fini() ;
               PD_RC_CHECK( rc, PDERROR, "Failed to finalize access plan "
                            "manager, rc: %d", rc ) ;

               // Initialize the cache again with new value of bucketNum
               rc = init( bucketNum, cacheLevel, sortBufferSize,
                          optCostThreshold, enableMixCmp, planCacheMainCLThreshold ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to initialize access plan "
                            "manager, rc: %d", rc ) ;
            }
         }
      }
      else
      {
         rc = init( bucketNum, cacheLevel, sortBufferSize,
                    optCostThreshold, enableMixCmp, planCacheMainCLThreshold ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize access plan manager, "
                      "rc: %d", rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTAPM_REINIT, rc ) ;
      return rc ;

   error :
      fini() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_FINI, "_optAccessPlanManager::fini" )
   INT32 _optAccessPlanManager::fini ()
   {
      PD_TRACE_ENTRY( SDB_OPTAPM_FINI ) ;

      if ( isInitialized() )
      {
         _planCache.disableCaching() ;

         _cacheLevel = OPT_PLAN_NOCACHE ;
         setMthEnableParameterized( FALSE ) ;
         setMthEnableFuzzyOptr( FALSE ) ;

         // Invalidate cached plans
         _planCache.invalidateAllPlans() ;

         // Finalize cache and monitor
         _planCache.deinitialize() ;
         _monitor.deinitialize() ;
      }

      PD_TRACE_EXIT( SDB_OPTAPM_FINI ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_GETAP, "_optAccessPlanManager::getAccessPlan" )
   INT32 _optAccessPlanManager::getAccessPlan ( const rtnQueryOptions &options,
                                                dmsStorageUnit *su,
                                                dmsMBContext *mbContext,
                                                optAccessPlanRuntime &planRuntime,
                                                const rtnExplainOptions *expOptions )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_GETAP ) ;

      SDB_ASSERT( su, "su is invalid" ) ;
      SDB_ASSERT( mbContext, "mbContext is invalid" ) ;

      BOOLEAN gotMainCLPlan = FALSE ;

      // Use main-collection plan in below case
      // 1. plan cache is initialized
      // 2. parameterized plan is enabled
      // 3. main-collection name is given
      // 4. path-searching is disabled
      // not use main-collection plan in below cases
      // - plan cache is not initialized
      //  collection is marked main-collection plan invalidated
      // - main-collection plan cache threshold is not enabled
      // - collection's data page number is less then main-collection plan cache threshold
      if ( isInitialized() &&
           _cacheLevel >= OPT_PLAN_PARAMETERIZED &&
           NULL != options.getMainCLName() &&
           ( NULL == expOptions || !expOptions->isNeedSearch() ) )
      {
         dmsCachedPlanMgr *pCachedPlanMgr = su->getCachedPlanMgr() ;
         INT32 mainCLThreshold = getPlanConfig()._planCacheMainThreshold ;
         if ( NULL == pCachedPlanMgr ||
              pCachedPlanMgr->testMainCLInvalidBitmap( mbContext->mbID() ) ||
              mainCLThreshold < 0 ||
              mbContext->mbStat()->_totalDataPages < (UINT32)( mainCLThreshold ) )
         {
            // The sub-collection is not validated to use main-collection plans,
            // generate a general plan for it
            gotMainCLPlan = FALSE ;
         }
         else
         {
            // If it is from main-collection, try to get or create main-collection
            // plan
            // Note: sub-collection name is considered as one of parameters
            rc = _getMainCLAccessPlan( options, su, mbContext, planRuntime ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get main-collection access plan "
                         "for query [ %s ], rc: %d", options.toString().c_str(),
                         rc ) ;
            gotMainCLPlan = TRUE ;
         }
      }

      if ( !gotMainCLPlan )
      {
         // If cache is not initialized, or it not from main-collection, or the
         // cache level is too low, get or create normal plan
         rc = _getCLAccessPlan( options, su, mbContext, planRuntime,
                                expOptions ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get collection access plan for "
                      "query [ %s ], rc: %d", options.toString().c_str(), rc ) ;
      }

#ifdef _DEBUG
      if ( OPT_PLAN_TYPE_MAINCL == planRuntime.getPlan()->getPlanType() )
      {
         PD_LOG( PDDEBUG, "Got main-collection plan [%s]",
                 planRuntime.getPlan()->toString().c_str() ) ;
      }
#endif

      // set explain options
      if ( NULL != expOptions )
      {
         planRuntime.setExplainOptions( expOptions ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTAPM_GETAP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_GETTEMPAP, "_optAccessPlanManager::getTempAccessPlan" )
   INT32 _optAccessPlanManager::getTempAccessPlan ( const rtnQueryOptions &options,
                                                    dmsStorageUnit *su,
                                                    dmsMBContext *mbContext,
                                                    optAccessPlanRuntime &planRuntime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_GETTEMPAP ) ;

      SDB_ASSERT( su, "su is invalid" ) ;
      SDB_ASSERT( mbContext, "mbContext is invalid" ) ;

      rc = _getCLAccessPlan( options, OPT_PLAN_NOCACHE, su, mbContext,
                             planRuntime, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get collection access plan for "
                   "query [ %s ], rc: %d", options.toString().c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTAPM_GETTEMPAP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_INVALIDCLPLANS, "_optAccessPlanManager::invalidateCLPlans" )
   void _optAccessPlanManager::invalidateCLPlans ( const CHAR *pCLFullName )
   {
      PD_TRACE_ENTRY( SDB_OPTAPM_INVALIDCLPLANS ) ;

      SDB_ASSERT( NULL != pCLFullName, "collection name is invalid" ) ;

      if ( isInitialized() )
      {
         _planCache.invalidateCLPlans( pCLFullName ) ;
      }

      PD_TRACE_EXIT( SDB_OPTAPM_INVALIDCLPLANS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_INVALIDSUPLANS, "_optAccessPlanManager::invalidateSUPlans" )
   void _optAccessPlanManager::invalidateSUPlans ( const CHAR *pCSName )
   {
      PD_TRACE_ENTRY( SDB_OPTAPM_INVALIDSUPLANS ) ;

      SDB_ASSERT( NULL != pCSName, "collection space name is invalid" ) ;

      if ( isInitialized() )
      {
         // Invalidate cached plans by collection space name
         _planCache.invalidateSUPlans( pCSName ) ;
      }

      PD_TRACE_EXIT( SDB_OPTAPM_INVALIDSUPLANS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_INVALIDALLPLANS, "_optAccessPlanManager::invalidateAllPlans" )
   void _optAccessPlanManager::invalidateAllPlans ()
   {
      PD_TRACE_ENTRY( SDB_OPTAPM_INVALIDALLPLANS ) ;

      if ( isInitialized() )
      {
         // Invalidate cached plans
         _planCache.invalidateAllPlans() ;
      }

      PD_TRACE_EXIT( SDB_OPTAPM_INVALIDALLPLANS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_SETQUERYACT, "_optAccessPlanManager::setQueryActivity" )
   void _optAccessPlanManager::setQueryActivity ( INT32 activityID,
                                                  const optQueryActivity &queryActivity,
                                                  const rtnParamList &parameters )
   {
      PD_TRACE_ENTRY( SDB_OPTAPM_SETQUERYACT ) ;

      if ( isInitialized() )
      {
         optCachedPlanActivity *activity = _monitor.getActivity( activityID ) ;
         if ( NULL != activity )
         {
            activity->setQueryActivity( queryActivity, parameters ) ;
         }
      }

      PD_TRACE_EXIT( SDB_OPTAPM_SETQUERYACT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_ONCRTCS, "_optAccessPlanManager::onCreateCS" )
   INT32 _optAccessPlanManager::onCreateCS ( IDmsEventHolder *pEventHolder,
                                             IDmsSUCacheHolder *pCacheHolder,
                                             pmdEDUCB *cb,
                                             SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_ONCRTCS ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      if ( pCacheHolder && isInitialized() )
      {
         _resetSUPlanCache( pCacheHolder ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTAPM_ONCRTCS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_ONLOADCS, "_optAccessPlanManager::onLoadCS" )
   INT32 _optAccessPlanManager::onLoadCS ( IDmsEventHolder *pEventHolder,
                                           IDmsSUCacheHolder *pCacheHolder,
                                           pmdEDUCB *cb,
                                           SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_ONLOADCS ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      if ( pCacheHolder && isInitialized() )
      {
         _resetSUPlanCache( pCacheHolder ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTAPM_ONLOADCS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_ONUNLOADCS, "_optAccessPlanManager::onUnloadCS" )
   INT32 _optAccessPlanManager::onUnloadCS ( IDmsEventHolder *pEventHolder,
                                             IDmsSUCacheHolder *pCacheHolder,
                                             pmdEDUCB *cb,
                                             SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_ONRENAMECL ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      if ( pCacheHolder && isInitialized() )
      {
         // Only to remove SU plans
         // no need to remove main-collection plans
         _invalidSUPlans( pCacheHolder ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTAPM_ONRENAMECL, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_ONRENAMECS, "_optAccessPlanManager::onRenameCS" )
   INT32 _optAccessPlanManager::onRenameCS ( IDmsEventHolder *pEventHolder,
                                             IDmsSUCacheHolder *pCacheHolder,
                                             const CHAR *pOldCSName,
                                             const CHAR *pNewCSName,
                                             pmdEDUCB *cb,
                                             SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_ONRENAMECS ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      if ( pCacheHolder && isInitialized() )
      {
         _invalidSUPlans( pCacheHolder ) ;

         // Run invalidation again to clear main-collection plans
         invalidateSUPlans( pOldCSName ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTAPM_ONRENAMECS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_ONDROPCS, "_optAccessPlanManager::onDropCS" )
   INT32 _optAccessPlanManager::onDropCS ( SDB_EVENT_OCCUR_TYPE type,
                                           IDmsEventHolder *pEventHolder,
                                           IDmsSUCacheHolder *pCacheHolder,
                                           const dmsEventSUItem &suItem,
                                           dmsDropCSOptions *options,
                                           pmdEDUCB *cb,
                                           SDB_DPSCB *dpsCB)
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_ONDROPCS ) ;

      if ( SDB_EVT_OCCUR_AFTER == type )
      {
         SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

         if ( pCacheHolder && isInitialized() )
         {
            _invalidSUPlans( pCacheHolder ) ;

            // Run invalidation again to clear main-collection plans
            invalidateSUPlans( pCacheHolder->getCSName() ) ;
         }
      }

      PD_TRACE_EXITRC( SDB_OPTAPM_ONDROPCS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_ONRENAMECL, "_optAccessPlanManager::onRenameCL" )
   INT32 _optAccessPlanManager::onRenameCL ( IDmsEventHolder *pEventHolder,
                                             IDmsSUCacheHolder *pCacheHolder,
                                             const dmsEventCLItem &clItem,
                                             const CHAR *pNewCLName,
                                             pmdEDUCB *cb,
                                             SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_ONRENAMECL ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      if ( pCacheHolder && isInitialized() )
      {
         _invalidCLPlans( pCacheHolder, clItem._mbID, clItem._clLID ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTAPM_ONRENAMECL, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_ONTRUNCATECL, "_optAccessPlanManager::onTruncateCL" )
   INT32 _optAccessPlanManager::onTruncateCL ( SDB_EVENT_OCCUR_TYPE type,
                                               IDmsEventHolder *pEventHolder,
                                               IDmsSUCacheHolder *pCacheHolder,
                                               const dmsEventCLItem &clItem,
                                               dmsTruncCLOptions *options,
                                               pmdEDUCB *cb,
                                               SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_ONTRUNCATECL ) ;

      if ( SDB_EVT_OCCUR_AFTER == type )
      {
         SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

         if ( pCacheHolder && isInitialized() )
         {
            _invalidCLPlans( pCacheHolder, clItem._mbID, clItem._clLID ) ;
         }
      }

      PD_TRACE_EXITRC( SDB_OPTAPM_ONTRUNCATECL, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_ONDROPCL, "_optAccessPlanManager::onDropCL" )
   INT32 _optAccessPlanManager::onDropCL ( SDB_EVENT_OCCUR_TYPE type,
                                           IDmsEventHolder *pEventHolder,
                                           IDmsSUCacheHolder *pCacheHolder,
                                           const dmsEventCLItem &clItem,
                                           dmsDropCLOptions *options,
                                           pmdEDUCB *cb,
                                           SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_ONDROPCL ) ;

      if ( SDB_EVT_OCCUR_BEFORE == type )
      {
         SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

         if ( pCacheHolder && isInitialized() )
         {
            _invalidCLPlans( pCacheHolder, clItem._mbID, clItem._clLID ) ;
         }
      }

      PD_TRACE_EXITRC( SDB_OPTAPM_ONDROPCL, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_ONREBUILDIDX, "_optAccessPlanManager::onRebuildIndex" )
   INT32 _optAccessPlanManager::onRebuildIndex ( IDmsEventHolder *pEventHolder,
                                                 IDmsSUCacheHolder *pCacheHolder,
                                                 const dmsEventCLItem &clItem,
                                                 const dmsEventIdxItem &idxItem,
                                                 pmdEDUCB *cb,
                                                 SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_ONREBUILDIDX ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      if ( pCacheHolder && isInitialized() )
      {
         _invalidCLPlans( pCacheHolder, clItem._mbID, clItem._clLID ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTAPM_ONREBUILDIDX, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_ONDROPIDX, "_optAccessPlanManager::onDropIndex" )
   INT32 _optAccessPlanManager::onDropIndex ( IDmsEventHolder *pEventHolder,
                                              IDmsSUCacheHolder *pCacheHolder,
                                              const dmsEventCLItem &clItem,
                                              const dmsEventIdxItem &idxItem,
                                              pmdEDUCB *cb,
                                              SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_ONDROPIDX ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      if ( pCacheHolder && isInitialized() )
      {
         _invalidCLPlans( pCacheHolder, clItem._mbID, clItem._clLID ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTAPM_ONDROPIDX, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_ONCLRSUCACHES, "_optAccessPlanManager::onClearSUCaches" )
   INT32 _optAccessPlanManager::onClearSUCaches ( IDmsEventHolder *pEventHolder,
                                                  IDmsSUCacheHolder *pCacheHolder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_ONCLRSUCACHES ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      if ( pCacheHolder && isInitialized() )
      {
         _invalidSUPlans( pCacheHolder ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTAPM_ONCLRSUCACHES, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_ONCLRCLCACHES, "_optAccessPlanManager::onClearCLCaches" )
   INT32 _optAccessPlanManager::onClearCLCaches ( IDmsEventHolder *pEventHolder,
                                                  IDmsSUCacheHolder *pCacheHolder,
                                                  const dmsEventCLItem &clItem )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_ONCLRCLCACHES ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      if ( pCacheHolder && isInitialized() )
      {
         _invalidCLPlans( pCacheHolder, clItem._mbID, clItem._clLID ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTAPM_ONCLRCLCACHES, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_ONCHGSUCACHES, "_optAccessPlanManager::onChangeSUCaches" )
   INT32 _optAccessPlanManager::onChangeSUCaches ( IDmsEventHolder *pEventHolder,
                                                   IDmsSUCacheHolder *pCacheHolder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_ONCHGSUCACHES ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      if ( pCacheHolder && isInitialized() )
      {
         _resetSUPlanCache( pCacheHolder ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTAPM_ONCHGSUCACHES, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM__GETCLAP, "_optAccessPlanManager::_getCLAccessPlan" )
   INT32 _optAccessPlanManager::_getCLAccessPlan ( const rtnQueryOptions &options,
                                                   dmsStorageUnit *su,
                                                   dmsMBContext *mbContext,
                                                   optAccessPlanRuntime &planRuntime,
                                                   const rtnExplainOptions *expOptions )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM__GETCLAP ) ;

      SDB_ASSERT( su, "su is invalid" ) ;
      SDB_ASSERT( mbContext, "mbContext is invalid" ) ;

      OPT_PLAN_CACHE_LEVEL cacheLevel = _cacheLevel ;

      if ( NULL != expOptions && expOptions->isNeedSearch() )
      {
         cacheLevel = OPT_PLAN_NOCACHE ;
      }

      // If the collection have been marked parameterized invalid,
      // lower the cache level to normalized
      if ( cacheLevel >= OPT_PLAN_PARAMETERIZED )
      {
         dmsCachedPlanMgr *pCachedPlanMgr = su->getCachedPlanMgr() ;
         if ( pCachedPlanMgr == NULL ||
              pCachedPlanMgr->testParamInvalidBitmap( mbContext->mbID() ) )
         {
            PD_LOG( PDDEBUG, "Collection [%s] is invalid for parameterized "
                    "plans", options.getCLFullName() ) ;
            cacheLevel = OPT_PLAN_NORMALIZED ;
         }
      }

      rc = _getCLAccessPlan( options, cacheLevel, su, mbContext, planRuntime,
                             expOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get collection access plan for "
                   "query [ %s ], rc: %d", options.toString().c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTAPM__GETCLAP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM__GETCLAP_LEVEL, "_optAccessPlanManager::_getCLAccessPlan" )
   INT32 _optAccessPlanManager::_getCLAccessPlan ( const rtnQueryOptions &options,
                                                   OPT_PLAN_CACHE_LEVEL cacheLevel,
                                                   dmsStorageUnit *su,
                                                   dmsMBContext *mbContext,
                                                   optAccessPlanRuntime &planRuntime,
                                                   const rtnExplainOptions *expOptions )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM__GETCLAP_LEVEL ) ;

      SDB_ASSERT( su, "su is invalid" ) ;
      SDB_ASSERT( mbContext, "mbContext is invalid" ) ;

      optGeneralAccessPlan *pPlan = NULL ;

      // Construct the plan key, but needn't to get owned at this stage
      optAccessPlanKey planKey( options, cacheLevel ) ;

      optAccessPlanHelper planHelper( cacheLevel, getPlanConfig(),
                                      getMatchConfig(), expOptions ) ;
      BOOLEAN needCache = ( isInitialized() &&
                            cacheLevel > OPT_PLAN_NOCACHE &&
                            !planHelper.isKeepPaths() ) ;

      planRuntime.reset() ;

      rc = _prepareAccessPlanKey( su, mbContext, planKey, planHelper,
                                  planRuntime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare key of access plan, rc: %d",
                   rc ) ;

      // recheck cache level
      needCache = ( planKey.getCacheLevel() > OPT_PLAN_NOCACHE ) ?
                  needCache : FALSE ;

      // If cache is initialized, try to get plan from cache first
      if ( needCache )
      {
         optAccessPlan *pTmpPlan = NULL ;

         rc = _getCachedAccessPlan( planKey, &pTmpPlan ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get access plan, rc: %d", rc ) ;
         if ( pTmpPlan )
         {
            pPlan = dynamic_cast<_optGeneralAccessPlan *>( pTmpPlan ) ;
            if ( NULL == pPlan )
            {
               // Cast failed, release the temp plan
               pTmpPlan->release() ;
            }
            else if ( pPlan->isEstimatedFromStat() &&
                      optCheckStatExpiredByPage( mbContext->mbStat()->_totalDataPages,
                                                 pPlan->getInputPages(),
                                                 planHelper.getOptCostThreshold(),
                                                 su->getPageSizeLog2() ) &&
                      optCheckStatExpiredBySize( mbContext->mbStat()->_totalOrgDataLen.fetch(),
                                                 pPlan->getInputRecordSize(),
                                                 planHelper.getOptCostThreshold(),
                                                 su->getPageSizeLog2() ) )
            {
               dmsCachedPlanMgr *pCachedPlanMgr = su->getCachedPlanMgr() ;

               // plan is expired
               PD_LOG( PDDEBUG, "Plan [%s] is expired, current pages [%d], "
                       "statistics pages [%d], current data size [%d], "
                       "statistics data size [%d], cost threshold [%d]",
                       pPlan->toString().c_str(),
                       mbContext->mbStat()->_totalDataPages,
                       pPlan->getInputPages(),
                       mbContext->mbStat()->_totalOrgDataLen.fetch(),
                       pPlan->getInputRecordSize(),
                       planHelper.getOptCostThreshold() ) ;

               // clear expired plan and flags
               _planCache.removeCachedPlan( pPlan, SHARED ) ;
               if ( NULL != pCachedPlanMgr )
               {
                  pCachedPlanMgr->clearParamInvalidBit( mbContext->mbID() ) ;
               }

               // release plan
               pPlan->release() ;
               pPlan = NULL ;
            }
         }
      }

      if ( NULL == pPlan )
      {
         // Failed to get plan from cache, create it
         rc = _createAccessPlan( su, mbContext, planKey, planRuntime,
                                 planHelper, &pPlan, needCache ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create access plan, rc: %d",
                      rc ) ;

         planRuntime.setPlan( pPlan, this, TRUE ) ;
         pPlan = NULL ;
      }
      else
      {
         if ( pPlan->getCacheLevel() >= OPT_PLAN_PARAMETERIZED )
         {
            optParamAccessPlan *paramPlan = dynamic_cast<optParamAccessPlan *>( pPlan ) ;
            SDB_ASSERT( paramPlan, "paramPlan is invalid" ) ;

            // Plan is parameterized, bind the parameters
            rc = _validateParamPlan( su, mbContext, planKey, planRuntime,
                                     planHelper, paramPlan ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to validate parameterized plan, "
                         "rc: %d", rc ) ;
         }
         else
         {
            // We don't need the match runtime any more
            // Use the one in the plan
            planRuntime.deleteMatchRuntime() ;
         }

         if ( planRuntime.hasPlan() )
         {
            pPlan->release() ;
            pPlan = NULL ;
         }
         else
         {
            planRuntime.setPlan( pPlan, this, FALSE ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTAPM__GETCLAP_LEVEL, rc ) ;
      return rc ;

   error :
      if ( NULL != pPlan )
      {
         pPlan->release() ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM__GETMAINAP, "_optAccessPlanManager::_getMainCLAccessPlan" )
   INT32 _optAccessPlanManager::_getMainCLAccessPlan ( const rtnQueryOptions &options,
                                                       dmsStorageUnit *su,
                                                       dmsMBContext *mbContext,
                                                       optAccessPlanRuntime &planRuntime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM__GETMAINAP ) ;

      SDB_ASSERT( su, "su is invalid" ) ;
      SDB_ASSERT( mbContext, "mbContext is invalid" ) ;
      SDB_ASSERT( options.getMainCLName(), "mainCLName is invalid" ) ;

      optAccessPlan *pPlan = NULL ;
      OPT_PLAN_CACHE_LEVEL cacheLevel = _cacheLevel ;
      UINT16 subCLMBID = mbContext->mbID() ;

      dmsCachedPlanMgr *pCachedPlanMgr = su->getCachedPlanMgr() ;

      if ( NULL == pCachedPlanMgr ||
           pCachedPlanMgr->testMainCLInvalidBitmap( subCLMBID ) )
      {
         // The sub-collection is not validated to use main-collection plans,
         // generate a general plan for it
         rc = _getCLAccessPlan( options, su, mbContext, planRuntime, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get collection access plan for "
                      "query [ %s ], rc: %d", options.toString().c_str(), rc ) ;
      }
      else
      {
         // Construct the plan key for main-collection
         optAccessPlanKey planKey( options, cacheLevel ) ;
         planKey.setCLFullName( options.getMainCLName() ) ;
         planKey.setMainCLName( NULL ) ;

         optAccessPlanHelper planHelper( cacheLevel, getPlanConfig(),
                                         getMatchConfig(), FALSE ) ;

         rc = _prepareAccessPlanKey( NULL, NULL, planKey, planHelper,
                                     planRuntime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to prepare key of access plan, "
                      "rc: %d", rc ) ;

         if ( planKey.getCacheLevel() > OPT_PLAN_NOCACHE )
         {
            // Try to get the main-collection plan from cache first
            rc = _getCachedAccessPlan( planKey, &pPlan ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get access plan, rc: %d", rc ) ;

            // if plan is main-collection validated, check whether statistics
            // for this sub-collection is expired
            if ( NULL != pPlan &&
                 pPlan->isMainCLValid() &&
                 pPlan->isEstimatedFromStat() &&
                 !pCachedPlanMgr->testParamInvalidBitmap( subCLMBID ) &&
                 optCheckStatExpiredByPage( mbContext->mbStat()->_totalDataPages,
                                            pPlan->getInputPages(),
                                            planHelper.getOptCostThreshold(),
                                            su->getPageSizeLog2() ) &&
                 optCheckStatExpiredBySize( mbContext->mbStat()->_totalOrgDataLen.fetch(),
                                            pPlan->getInputRecordSize(),
                                            planHelper.getOptCostThreshold(),
                                            su->getPageSizeLog2() ) )
            {
               // plan is expired
               PD_LOG( PDDEBUG, "Plan [%s] is expired, current pages [%d], "
                       "statistics pages [%d], current data size [%d], "
                       "statistics data size [%d], cost threshold [%d]",
                       pPlan->toString().c_str(),
                       mbContext->mbStat()->_totalDataPages,
                       pPlan->getInputPages(),
                       mbContext->mbStat()->_totalOrgDataLen.fetch(),
                       pPlan->getInputRecordSize(),
                       planHelper.getOptCostThreshold() ) ;

               // clear expired plan and flags
               _planCache.removeCachedPlan( pPlan, SHARED ) ;
               pCachedPlanMgr->clearMainCLInvalidBit( subCLMBID ) ;

               // release plan
               pPlan->release() ;
               pPlan = NULL ;
            }
         }

         if ( NULL == pPlan )
         {
            if ( planKey.getCacheLevel() > OPT_PLAN_NOCACHE )
            {
               optMainCLAccessPlan * mainPlan = NULL ;

               // Could not find the main-collection plan from cache, so create
               // the plan for sub-collection and bind it to the main-collection
               // plan
               rc = _createMainCLPlan( planKey, options, su, mbContext,
                                       planRuntime, planHelper, &mainPlan ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to create main-collection "
                            "query, rc: %d", rc ) ;

               // we won't cache the main-collection plan in below cases
               // - the hint is failed, which means some indexes may not exist
               //   in current sub-collection
               // - the plan is table scan, which means the current
               //   sub-collection does not have matched index, but this does
               //   not mean other sub-collections do not have
               if ( ( !mainPlan->isHintFailed() ) &&
                    ( IXSCAN == mainPlan->getScanType() ) )
               {
                  _cacheAccessPlan( mainPlan ) ;
               }

               // Use the sub-collection plan for the this time
               mainPlan->release() ;
            }
            else
            {
               // no cache, create a generate plan
               optGeneralAccessPlan * generalPlan = NULL ;

               // use the sub-collection to create plan
               planKey.setCLFullName( options.getCLFullName() ) ;
               planKey.setCollectionInfo( su, mbContext ) ;
               rc = _createAccessPlan( su, mbContext, planKey, planRuntime,
                                       planHelper, &generalPlan, FALSE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to create access plan, rc: %d",
                            rc ) ;

               planRuntime.setPlan( generalPlan, this, TRUE ) ;
               generalPlan = NULL ;
            }
         }
         else
         {
            optMainCLAccessPlan * mainPlan =
                              dynamic_cast<optMainCLAccessPlan *>( pPlan ) ;
            SDB_ASSERT( mainPlan, "mainPlan is invalid " ) ;

            if ( pCachedPlanMgr->testParamInvalidBitmap( subCLMBID ) ||
                 !mainPlan->isMainCLValid() )
            {
               // The sub-collection is not parameterized validated, we need
               // to verify if it is validate to main-collection plan
               rc = _validateMainCLPlan( mainPlan, options, su, mbContext,
                                         planRuntime, planHelper ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to validate main-collection "
                            "plan, rc: %d", rc ) ;
            }
            else
            {
               rc = _bindMainCLPlan( mainPlan, options, su, mbContext,
                                     planRuntime, planHelper ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to bind main-collection "
                            "plan, rc: %d", rc ) ;
            }

            if ( planRuntime.hasPlan() )
            {
               // Already got one plan, release this one
               pPlan->release() ;
               pPlan = NULL ;
            }
            else
            {
               // Set the plan
               planRuntime.setPlan( pPlan, this, FALSE ) ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTAPM__GETMAINAP, rc ) ;
      return rc ;

   error :
      if ( NULL != pPlan )
      {
         pPlan->release() ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM__PREPAREAPKEY, "_optAccessPlanManager::_prepareAccessPlanKey" )
   INT32 _optAccessPlanManager::_prepareAccessPlanKey ( dmsStorageUnit *su,
                                                        dmsMBContext *mbContext,
                                                        optAccessPlanKey &planKey,
                                                        optAccessPlanHelper &planHelper,
                                                        optAccessPlanRuntime &planRuntime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM__PREPAREAPKEY ) ;

#ifdef _DEBUG
      PD_LOG( PDDEBUG, "Original query: [%s] %s", planKey.getCLFullName(),
              planKey.getQuery().toString( FALSE, TRUE ).c_str() ) ;
#endif

      if ( planKey.getCacheLevel() >= OPT_PLAN_NORMALIZED )
      {
         // Normalize the query
         rc = planRuntime.createMatchRuntime() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create match runtime, rc: %d",
                      rc ) ;

         rc = planKey.normalize( planHelper, planRuntime.getMatchRuntime() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to normalize plan key, rc: %d", rc ) ;

         if ( planKey.getCacheLevel() >= OPT_PLAN_NORMALIZED )
         {
#ifdef _DEBUG
            PD_LOG( PDDEBUG, "Normalized query: [%s] %s Params: %s",
                    planKey.getCLFullName(),
                    planKey.getNormalizedQuery().toString( FALSE, TRUE ).c_str(),
                    planRuntime.getParameters().toString().c_str() ) ;
#endif
         }
         else
         {
            // The match runtime is not needed
            planRuntime.deleteMatchRuntime() ;
         }
      }

      planKey.setCollectionInfo( su, mbContext ) ;

   done :
      PD_TRACE_EXITRC( SDB_OPTAPM__PREPAREAPKEY, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM__CRTAP, "_optAccessPlanManager::_createAccessPlan" )
   INT32 _optAccessPlanManager::_createAccessPlan ( dmsStorageUnit *su,
                                                    dmsMBContext *mbContext,
                                                    optAccessPlanKey &planKey,
                                                    optAccessPlanRuntime &planRuntime,
                                                    optAccessPlanHelper &planHelper,
                                                    optGeneralAccessPlan **ppPlan,
                                                    BOOLEAN needCache )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM__CRTAP ) ;

      SDB_ASSERT( su, "su is invalid" ) ;
      SDB_ASSERT( mbContext, "mbContext is invalid" ) ;
      SDB_ASSERT( ppPlan, "ppPlan is invalid" ) ;

      optGeneralAccessPlan *pPlan = NULL ;
      BOOLEAN isParameterized = needCache &&
                                planKey.getCacheLevel() >= OPT_PLAN_PARAMETERIZED ;

      if ( isParameterized )
      {
         pPlan = SDB_OSS_NEW optParamAccessPlan( planKey,
                                                 planHelper.getMatchConfig() ) ;
      }
      else
      {
         pPlan = SDB_OSS_NEW optGeneralAccessPlan( planKey,
                                                   planHelper.getMatchConfig() ) ;
      }
      PD_CHECK( NULL != pPlan, SDB_OOM, error, PDERROR,
                "Not able to allocate memory for new plan" ) ;

      rc = pPlan->getKeyOwned() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key of access plan owned, "
                   "rc: %d", rc ) ;

      if ( NULL == planRuntime.getMatchRuntime() )
      {
         rc = pPlan->createMatchRuntime() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create match runtime, rc: %d",
                      rc ) ;
         planRuntime.setMatchRuntime( pPlan->getMatchRuntime() ) ;
      }
      else
      {
         pPlan->getMatchRuntimeOnwed( planRuntime ) ;
      }

      rc = pPlan->optimize( su, mbContext, planHelper ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to optimize plan, query: %s\norder %s\nhint %s",
                   planKey.getQuery().toString().c_str(),
                   planKey.getOrderBy().toString().c_str(),
                   planKey.getHint().toString().c_str() ) ;

      if ( isParameterized )
      {
         // Validate self
         BSONObj parameters = planRuntime.getParameters().toBSON() ;
         pPlan->validateParameterized( *pPlan, parameters ) ;
      }

      // Set the outputs
      (*ppPlan) = pPlan ;

      // Cache the plan
      if ( needCache && isInitialized() )
      {
         _cacheAccessPlan( pPlan ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTAPM__CRTAP, rc ) ;
      return rc ;

   error :
      if ( NULL != pPlan )
      {
         pPlan->release() ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM__GETCACHEDAP, "_optAccessPlanManager::_getCachedAccessPlan" )
   INT32 _optAccessPlanManager::_getCachedAccessPlan ( const optAccessPlanKey &planKey,
                                                       optAccessPlan **ppPlan )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM__GETCACHEDAP ) ;

      SDB_ASSERT( ppPlan, "ppPlan is invalid" ) ;

      optAccessPlan *pPlan = NULL ;
      pPlan = (optAccessPlan *)_planCache.getItem( planKey ) ;
      (*ppPlan) = pPlan ;

      PD_TRACE_EXITRC( SDB_OPTAPM__GETCACHEDAP, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM__CACHEAP, "_optAccessPlanManager::_cacheAccessPlan" )
   BOOLEAN _optAccessPlanManager::_cacheAccessPlan ( optAccessPlan *pPlan )
   {
      BOOLEAN cached = FALSE ;

      PD_TRACE_ENTRY( SDB_OPTAPM__CACHEAP ) ;

      if ( !pPlan->canCache() )
      {
         goto done ;
      }

      cached = _planCache.addPlan( pPlan ) ;
      if ( cached )
      {
         if ( !_monitor.setActivity( pPlan ) )
         {
            // Could not allocate activity for the plan
            // remove it from cache
            if ( _planCache.removeItem( pPlan ) )
            {
               cached = FALSE ;
            }
         }
         else
         {
            // Re-check if the plan is still cached.
            // If it is not cached after setting the activity, it might be
            // removed by dropCL, so we need to reset the activity if the
            // dropCL didn't
            if ( !pPlan->isCached() )
            {
               _planCache.resetCachedPlanActivity( pPlan, SHARED ) ;
               cached = FALSE ;
            }
         }
      }

   done:
      PD_TRACE_EXIT( SDB_OPTAPM__CACHEAP ) ;
      return cached ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM__VALIDPARAMPLAN, "_optAccessPlanManager::_validateParamPlan" )
   INT32 _optAccessPlanManager::_validateParamPlan ( dmsStorageUnit *su,
                                                     dmsMBContext *mbContext,
                                                     optAccessPlanKey &planKey,
                                                     optAccessPlanRuntime &planRuntime,
                                                     optAccessPlanHelper &planHelper,
                                                     optParamAccessPlan *plan )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM__VALIDPARAMPLAN ) ;

      SDB_ASSERT ( NULL != plan &&
                   plan->getCacheLevel() >= OPT_PLAN_PARAMETERIZED,
                   "plan is invalid" ) ;

      BSONObj parameters ;
      optGeneralAccessPlan *tempPlan = NULL ;

      // access plan is parameterized validated
      if ( plan->isParamValid() )
      {
         rc = planRuntime.bindParamPlan( planHelper, plan ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to bind parameterized plan, rc: %d",
                      rc ) ;
         goto done ;
      }

      // access plan has the same parameters
      parameters = planRuntime.getParameters().toBSON() ;
      if ( plan->checkSavedParam( parameters ) )
      {
         rc = planRuntime.bindParamPlan( planHelper, plan ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to bind parameterized plan, rc: %d",
                      rc ) ;
         goto done ;
      }

      rc = _getCLAccessPlan( planKey, OPT_PLAN_NOCACHE, su, mbContext,
                             planRuntime, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get access plan for with "
                   "query [ %s ], rc: %d", planKey.toString().c_str(), rc ) ;

      tempPlan = dynamic_cast<optGeneralAccessPlan *>( planRuntime.getPlan() ) ;
      SDB_ASSERT( tempPlan, "subPlan is invalid " ) ;

      if ( plan->validateParameterized( *tempPlan, parameters ) )
      {
         // Do nothing
      }
      else
      {
         // The parameter is not validate for the parameterized
         // plan, mark the collection invalidate for parameterized plans
         PD_LOG( PDDEBUG, "Invalid parameterized plan [%s]",
                 plan->toString().c_str() ) ;
         plan->markParamInvalid( mbContext ) ;
         _planCache.removeCachedPlan( plan, SHARED ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTAPM__VALIDPARAMPLAN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM__CRTMAINCLPLAN, "_optAccessPlanManager::_createMainCLPlan" )
   INT32 _optAccessPlanManager::_createMainCLPlan ( optAccessPlanKey &planKey,
                                                    const rtnQueryOptions &subOptions,
                                                    dmsStorageUnit *su,
                                                    dmsMBContext *mbContext,
                                                    optAccessPlanRuntime &planRuntime,
                                                    optAccessPlanHelper &planHelper,
                                                    optMainCLAccessPlan **ppPlan )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM__CRTMAINCLPLAN ) ;

      SDB_ASSERT( su, "su is invalid" ) ;
      SDB_ASSERT( mbContext, "mbContext is invalid" ) ;
      SDB_ASSERT( ppPlan, "ppPlan is invalid" ) ;

      optMainCLAccessPlan *mainPlan = NULL ;
      optGeneralAccessPlan *subPlan = NULL ;
      BSONObj parameters ;

      mainPlan = SDB_OSS_NEW optMainCLAccessPlan( planKey,
                                                  planHelper.getMatchConfig() ) ;
      PD_CHECK( mainPlan, SDB_OOM, error, PDERROR,
                "Failed to allocate main-collection access plan" ) ;

      // Get the key owned
      rc = mainPlan->getKeyOwned() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key of access plan owned, "
                   "rc: %d", rc ) ;

      // Make sure matcher runtime is created, and must owned by plan
      if ( NULL == planRuntime.getMatchRuntime() )
      {
         rc = mainPlan->createMatchRuntime() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create match runtime, "
                      "rc: %d", rc ) ;
         planRuntime.setMatchRuntime( mainPlan->getMatchRuntime() ) ;
      }
      else
      {
         mainPlan->getMatchRuntimeOnwed( planRuntime ) ;
      }

      // Save parameters
      if ( !planRuntime.getParameters().isEmpty() )
      {
         parameters = planRuntime.getParameters().toBSON() ;
      }

      // Prepare to bind the sub-collection plan
      rc = mainPlan->prepareBindSubCL( planHelper ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare main-collection "
                   "plan, rc: %d", rc ) ;

      // Generate the sub-collection plan
      // Specify the cache level, APM is not allowed to adjust it
      rc = _getCLAccessPlan( subOptions, OPT_PLAN_NOCACHE, su, mbContext,
                             planRuntime, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get access plan for "
                   "sub-collection with query [ %s ], rc: %d",
                   subOptions.toString().c_str(), rc ) ;

      // Bind the sub-collection plan
      subPlan = dynamic_cast<optGeneralAccessPlan *>( planRuntime.getPlan() ) ;
      SDB_ASSERT( subPlan, "subPlan is invalid " ) ;

      rc = mainPlan->bindSubCLAccessPlan( planHelper,
                                          subPlan,
                                          mbContext,
                                          parameters ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to bind main-collection access "
                   "plan, rc: %d" ) ;

      (*ppPlan) = mainPlan ;

   done :
      PD_TRACE_EXITRC( SDB_OPTAPM__CRTMAINCLPLAN, rc ) ;
      return rc ;

   error :
      if ( NULL != mainPlan )
      {
         mainPlan->release() ;
      }
      if ( NULL != subPlan )
      {
         subPlan->release() ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM__VALIDMAINCLPLAN, "_optAccessPlanManager::_validateMainCLPlan" )
   INT32 _optAccessPlanManager::_validateMainCLPlan ( optMainCLAccessPlan *mainPlan,
                                                      const rtnQueryOptions &subOptions,
                                                      dmsStorageUnit *su,
                                                      dmsMBContext *mbContext,
                                                      optAccessPlanRuntime &planRuntime,
                                                      optAccessPlanHelper &planHelper )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM__VALIDMAINCLPLAN ) ;

      SDB_ASSERT( su, "su is invalid" ) ;
      SDB_ASSERT( mbContext, "mbContext is invalid" ) ;

      // The sub-collection is not parameterized validated, we need
      // to verify if it is validate to main-collection plan
      optGeneralAccessPlan *subPlan = NULL ;
      dmsCachedPlanMgr *pCachedPlanMgr = su->getCachedPlanMgr() ;
      BSONObj parameters ;

      // Save parameters
      if ( mainPlan->getCacheLevel() >= OPT_PLAN_PARAMETERIZED &&
           !planRuntime.getParameters().isEmpty() )
      {
         parameters = planRuntime.getParameters().toBSON() ;
      }

      // Check whether the sub-collection and parameters had been
      // already validated
      if ( mainPlan->checkSavedSubCL( mbContext->mb()->_clUniqueID,
                                      parameters ) )
      {
         rc = _bindMainCLPlan( mainPlan, subOptions, su, mbContext,
                               planRuntime, planHelper ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to bind main-collection "
                      "plan, rc: %d", rc ) ;
         goto done ;
      }

      // Specify the cache level, APM is not allowed to adjust it
      rc = _getCLAccessPlan( subOptions, OPT_PLAN_NOCACHE, su, mbContext,
                             planRuntime, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get access plan for "
                   "sub-collection with query [ %s ], rc: %d",
                   subOptions.toString().c_str(), rc ) ;

      subPlan = dynamic_cast<optGeneralAccessPlan *>( planRuntime.getPlan() ) ;
      SDB_ASSERT( subPlan, "subPlan is invalid " ) ;

      if ( !mainPlan->validateSubCLPlan( subPlan, mbContext, parameters ) )
      {
         // The sub-collection is not validate for the main-collection
         // plan, mark the collection invalidate for main-collection plans
         PD_LOG( PDDEBUG, "Invalid main-collection plan [%s]",
                 mainPlan->toString().c_str() ) ;
         mainPlan->markMainCLInvalid( pCachedPlanMgr, mbContext, FALSE ) ;
         _planCache.removeCachedPlan( mainPlan, SHARED ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTAPM__VALIDMAINCLPLAN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM__BINDMAINCLPLAN, "_optAccessPlanManager::_bindMainCLPlan" )
   INT32 _optAccessPlanManager::_bindMainCLPlan ( optMainCLAccessPlan *mainPlan,
                                                  const rtnQueryOptions &subOptions,
                                                  dmsStorageUnit *su,
                                                  dmsMBContext *mbContext,
                                                  optAccessPlanRuntime &planRuntime,
                                                  optAccessPlanHelper &planHelper )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM__BINDMAINCLPLAN ) ;

      SDB_ASSERT( mainPlan, "mainPlan is invalid" ) ;
      SDB_ASSERT( su, "su is invalid" ) ;
      SDB_ASSERT( mbContext, "mbContext is invalid" ) ;

      dmsCachedPlanMgr *pCachedPlanMgr = su->getCachedPlanMgr() ;
      dmsExtentID indexExtID = DMS_INVALID_EXTENT ;
      dmsExtentID indexLID = DMS_INVALID_EXTENT ;

      // The sub-collection is parameterized validated, we need
      // to verify if it has the index specified by main-colleciton
      // plan, etc
      rc = mainPlan->validateSubCL( su, mbContext, indexExtID, indexLID ) ;
      if ( SDB_OK != rc )
      {
         // Failed to validate sub-collection, generate a general plan
         // for sub-collection ( e.g. missing index )
         rc = mainPlan->markMainCLInvalid( pCachedPlanMgr,
                                           mbContext,
                                           TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to mark sub-collection "
                      "invalidated to reuse main-collection plan, "
                      "rc: %d", rc ) ;

         // Create a general plan for sub-collection
         rc = _getCLAccessPlan( subOptions, su, mbContext, planRuntime, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get collection access plan for "
                      "query [ %s ], rc: %d", subOptions.toString().c_str(),
                      rc ) ;

         goto done ;
      }

      // Bind plan info ( suID, mbID, etc )
      rc = planRuntime.bindPlanInfo( subOptions.getCLFullName(), su, mbContext,
                                     indexExtID, indexLID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to bind plan info, rc: %d",
                   rc ) ;

      // Bind parameters
      if ( mainPlan->getCacheLevel() >= OPT_PLAN_PARAMETERIZED )
      {
         rc = planRuntime.bindParamPlan( planHelper, mainPlan ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to bind parameterized "
                      "plan, rc: %d", rc ) ;
      }
      else
      {
         // We don't need the match runtime any more
         // Use the one in the plan
         planRuntime.deleteMatchRuntime() ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTAPM__BINDMAINCLPLAN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM__INVALIDPLANS, "_optAccessPlanManager::_invalidSUPlans" )
   void _optAccessPlanManager::_invalidSUPlans ( IDmsSUCacheHolder *pCacheHolder )
   {
      PD_TRACE_ENTRY( SDB_OPTAPM__INVALIDPLANS ) ;

      SDB_ASSERT( pCacheHolder, "pCacheHolder is invalid" ) ;

      dmsCachedPlanMgr *pCachedPlanMgr =
            (dmsCachedPlanMgr *)pCacheHolder->getSUCache( DMS_CACHE_TYPE_PLAN ) ;
      if ( pCachedPlanMgr )
      {
         UINT32 suLID = pCacheHolder->getSULID() ;

         _planCache.invalidateSUPlans( pCachedPlanMgr, suLID ) ;

         // No plans belong to this SU, clear the bitmap and free the units
         pCachedPlanMgr->resetCacheBitmap() ;
         pCachedPlanMgr->resetParamInvalidBitmap() ;
         pCachedPlanMgr->resetMainCLInvalidBitmap() ;
         pCachedPlanMgr->clearCacheUnits() ;
      }
      PD_TRACE_EXIT( SDB_OPTAPM__INVALIDPLANS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM__INVALIDCLPLANS, "_optAccessPlanManager::_invalidCLPlans" )
   void _optAccessPlanManager::_invalidCLPlans ( IDmsSUCacheHolder *pCacheHolder,
                                                 UINT16 mbID, UINT32 clLID )
   {
      PD_TRACE_ENTRY( SDB_OPTAPM__INVALIDCLPLANS ) ;

      SDB_ASSERT( pCacheHolder, "pCacheHolder is invalid" ) ;

      dmsCachedPlanMgr *pCachedPlanMgr =
            (dmsCachedPlanMgr *)pCacheHolder->getSUCache( DMS_CACHE_TYPE_PLAN ) ;
      if ( pCachedPlanMgr )
      {
         UINT32 suLID = pCacheHolder->getSULID() ;

         _planCache.invalidateCLPlans( pCachedPlanMgr, suLID, clLID ) ;

         pCachedPlanMgr->clearParamInvalidBit( mbID ) ;
         pCachedPlanMgr->clearMainCLInvalidBit( mbID ) ;
         pCachedPlanMgr->removeCacheUnit( mbID, TRUE ) ;
      }

      PD_TRACE_EXIT( SDB_OPTAPM__INVALIDCLPLANS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM__RESETSUCACHE, "_optAccessPlanManager::_resetSUPlanCache" )
   void _optAccessPlanManager::_resetSUPlanCache ( IDmsSUCacheHolder *pCacheHolder )
   {
      PD_TRACE_ENTRY( SDB_OPTAPM__RESETSUCACHE ) ;

      SDB_ASSERT( pCacheHolder, "pCacheHolder is invalid" ) ;

      dmsCachedPlanMgr *pCachedPlanMgr =
                           dynamic_cast<dmsCachedPlanMgr *>(pCacheHolder) ;
      if ( pCachedPlanMgr )
      {
         pCachedPlanMgr->resizeBitmaps( _planCache.getBucketNum() ) ;
         pCachedPlanMgr->resetParamInvalidBitmap() ;
         pCachedPlanMgr->resetMainCLInvalidBitmap() ;
         pCachedPlanMgr->clearCacheUnits() ;
      }

      PD_TRACE_EXIT( SDB_OPTAPM__RESETSUCACHE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM__STARTCLEARJOB, "_optAccessPlanManager::_startClearJob" )
   INT32 _optAccessPlanManager::_startClearJob ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM__STARTCLEARJOB ) ;

      rc = startPlanClearJob( &_clearJobEduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Start cached-plan clearing job thread "
                   "failed, rc: %d", rc ) ;
   done :
      PD_TRACE_EXITRC( SDB_OPTAPM__STARTCLEARJOB, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM__STOPCLEARJOB, "_optAccessPlanManager::_stopClearJob" )
   void _optAccessPlanManager::_stopClearJob ()
   {
      if ( PMD_INVALID_EDUID != _clearJobEduID )
      {
         pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;

         if ( !eduMgr->isDestroyed() )
         {
            eduMgr->forceUserEDU( _clearJobEduID ) ;
         }

         _clearJobEduID = PMD_INVALID_EDUID ;
      }
   }

}
