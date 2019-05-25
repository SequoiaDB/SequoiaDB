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

      result = addItem( pPlan ) ;

      PD_TRACE_EXIT( SDB_OPTAPCACHES_ADDPLAN ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPCACHES_RMPLAN, "_optAccessPlanCache::removeCachedPlan" )
   void _optAccessPlanCache::removeCachedPlan ( optAccessPlan *pPlan )
   {
      PD_TRACE_ENTRY( SDB_OPTAPCACHES_RMPLAN ) ;

      pPlan->incRefCount() ;
      if ( removeItem( pPlan ) )
      {
         _pMonitor->resetActivity( pPlan->resetActivityID() ) ;
         _pMonitor->decCachedPlanCount( 1 ) ;
      }
      pPlan->release() ;

      PD_TRACE_EXIT( SDB_OPTAPCACHES_RMPLAN ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPCACHES_INVALIDSUPLANS, "_optAccessPlanCache::invalidateSUPlans" )
   void _optAccessPlanCache::invalidateSUPlans ( dmsCachedPlanMgr *pCachedPlanMgr,
                                                 UINT32 suLID )
   {
      PD_TRACE_ENTRY( SDB_OPTAPCACHES_INVALIDSUPLANS ) ;

      SDB_ASSERT( pCachedPlanMgr, "pCachedPlanMgr is invalid" ) ;

      UINT32 deleteCount = 0 ;

      ossScopedRWLock scopedLock( &_bucketNumLock, SHARED ) ;

      for ( UINT32 bucketID = 0 ; bucketID < getBucketNum() ; bucketID ++ )
      {
         if ( pCachedPlanMgr->getBucketNum() != getBucketNum() ||
              pCachedPlanMgr->testCacheBitmap( bucketID ) )
         {
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
                     pPlan->incRefCount() ;

                     if ( pBucket->removeItem( pPlan ) )
                     {
                        _pMonitor->resetActivity( pPlan->resetActivityID() ) ;
                        deleteCount ++ ;
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

      if ( deleteCount > 0 )
      {
         _pMonitor->decCachedPlanCount( deleteCount ) ;
      }

      PD_TRACE_EXIT( SDB_OPTAPCACHES_INVALIDSUPLANS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPCACHES_INVALIDCLPLANS, "_optAccessPlanCache::invalidateCLPlans" )
   void _optAccessPlanCache::invalidateCLPlans ( dmsCachedPlanMgr *pCachedPlanMgr,
                                                 UINT32 suLID, UINT32 clLID )
   {
      PD_TRACE_ENTRY( SDB_OPTAPCACHES_INVALIDCLPLANS ) ;

      SDB_ASSERT( pCachedPlanMgr, "pCachedPlanMgr is invalid" ) ;

      UINT32 deleteCount = 0 ;

      ossScopedRWLock scopedLock( &_bucketNumLock, SHARED ) ;

      for ( UINT32 bucketID = 0 ; bucketID < getBucketNum() ; bucketID ++ )
      {
         if ( pCachedPlanMgr->getBucketNum() != getBucketNum() ||
              pCachedPlanMgr->testCacheBitmap( bucketID ) )
         {
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
                     pPlan->incRefCount() ;

                     if ( pBucket->removeItem( pPlan ) )
                     {
                        _pMonitor->resetActivity( pPlan->resetActivityID() ) ;
                        deleteCount ++ ;
                     }
                     else if ( clearBit )
                     {
                        clearBit = FALSE ;
                     }

                     pPlan->release() ;
                  }
                  else if ( clearBit && pPlan->getSULID() == suLID )
                  {
                     clearBit = FALSE ;
                  }
                  pPlan = pNextPlan ;
               }

               if ( clearBit )
               {
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

      if ( deleteCount > 0 )
      {
         _pMonitor->decCachedPlanCount( deleteCount ) ;
      }

      PD_TRACE_EXIT( SDB_OPTAPCACHES_INVALIDCLPLANS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPCACHES_INVALIDALLPLANS, "_optAccessPlanCache::invalidateAllPlans" )
   void _optAccessPlanCache::invalidateAllPlans ()
   {
      PD_TRACE_ENTRY( SDB_OPTAPCACHES_INVALIDALLPLANS ) ;

      UINT32 deleteCount = 0 ;

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
               pPlan->incRefCount() ;

               if ( pBucket->removeItem( pPlan ) )
               {
                  _pMonitor->resetActivity( pPlan->resetActivityID() ) ;
                  deleteCount ++ ;
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

      if ( deleteCount > 0 )
      {
         _pMonitor->decCachedPlanCount( deleteCount ) ;
      }

      PD_TRACE_EXIT( SDB_OPTAPCACHES_INVALIDALLPLANS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPCACHES_INVALIDCLPLANS_NAME, "_optAccessPlanCache::invalidateCLPlans" )
   void _optAccessPlanCache::invalidateCLPlans ( const CHAR *pCLFullName )
   {
      PD_TRACE_ENTRY( SDB_OPTAPCACHES_INVALIDCLPLANS_NAME ) ;

      SDB_ASSERT( NULL != pCLFullName, "collection name is invalid" ) ;

      UINT32 deleteCount = 0 ;

      ossScopedRWLock scopedLock( &_bucketNumLock, SHARED ) ;

      for ( UINT32 bucketID = 0 ; bucketID < getBucketNum() ; bucketID ++ )
      {
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
                  pPlan->incRefCount() ;

                  if ( pBucket->removeItem( pPlan ) )
                  {
                     _pMonitor->resetActivity( pPlan->resetActivityID() ) ;
                     deleteCount ++ ;
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

      if ( deleteCount > 0 )
      {
         _pMonitor->decCachedPlanCount( deleteCount ) ;
      }

      PD_TRACE_EXIT( SDB_OPTAPCACHES_INVALIDCLPLANS_NAME ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPCACHES_INVALIDSUPLANS_NAME, "_optAccessPlanCache::invalidateSUPlans" )
   void _optAccessPlanCache::invalidateSUPlans ( const CHAR * pCSName )
   {
      PD_TRACE_ENTRY( SDB_OPTAPCACHES_INVALIDSUPLANS_NAME ) ;

      SDB_ASSERT( NULL != pCSName, "collection space name is invalid" ) ;

      UINT32 deleteCount = 0 ;

      ossScopedRWLock scopedLock( &_bucketNumLock, SHARED ) ;

      for ( UINT32 bucketID = 0 ; bucketID < getBucketNum() ; bucketID ++ )
      {
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
                  pPlan->incRefCount() ;

                  if ( pBucket->removeItem( pPlan ) )
                  {
                     _pMonitor->resetActivity( pPlan->resetActivityID() ) ;
                     deleteCount ++ ;
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

      if ( deleteCount > 0 )
      {
         _pMonitor->decCachedPlanCount( deleteCount ) ;
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
               PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection space "
                            "name, rc: %d", rc ) ;

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

               pPlan->toBSON( planBuilder ) ;

               if ( NULL != _pMonitor )
               {
                  const optCachedPlanActivity *activity =
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
               rc = SDB_SYS ;
               goto error ;
            }

            pPlan = (optAccessPlan *)pPlan->getNext() ;
         }

         releaseBucket( bucketID, SHARED ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTCPCACHE_GETCPLIST, rc ) ;
      return rc ;
   error :
      goto done ;
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
   }

   void _optAccessPlanCache::_afterGetItem ( UINT32 bucketID,
                                             optAccessPlan *pPlan )
   {
      SDB_ASSERT( pPlan, "pPlan is invalid" ) ;

      pPlan->incRefCount() ;
      _pMonitor->setCachedPlanActivity( pPlan ) ;
   }

   /*
      _optCachedPlanActivity implement
    */
   _optCachedPlanActivity::_optCachedPlanActivity ()
   : _pPlan( NULL ),
     _lastAccessTime( 0 ),
     _periodAccessCount( 0 ),
     _accessCount( 0 ),
     _totalQueryTimeTick()
   {
   }

   _optCachedPlanActivity::~_optCachedPlanActivity ()
   {
   }

   void _optCachedPlanActivity::clear ()
   {
      _lastAccessTime = 0 ;
      _periodAccessCount = 0 ;
      _accessCount = 0 ;
      _totalQueryTimeTick.clear() ;
      _maxQueryActivity.reset() ;
      _minQueryActivity.reset() ;
      _pPlan = NULL ;
   }

   void _optCachedPlanActivity::setPlan ( optAccessPlan *pPlan,
                                          UINT64 timestamp )
   {
      _pPlan = pPlan ;
      _lastAccessTime = timestamp ;
      _periodAccessCount = 0 ;
      _accessCount = 0 ;
      _totalQueryTimeTick.clear() ;
      _maxQueryActivity.reset() ;
      _minQueryActivity.reset() ;
   }

   void _optCachedPlanActivity::setQueryActivity (
                                    const optQueryActivity &queryActivity )
   {
      if ( !isEmpty() )
      {
         _totalQueryTimeTick += queryActivity.getQueryTime() ;
         if ( queryActivity.getQueryTime() < _minQueryActivity.getQueryTime() ||
              !_minQueryActivity.isValid() )
         {
            _minQueryActivity = queryActivity ;
         }
         if ( queryActivity.getQueryTime() > _maxQueryActivity.getQueryTime() ||
              !_maxQueryActivity.isValid() )
         {
            _maxQueryActivity = queryActivity ;
         }
         incAccessCount() ;
      }
   }

   void _optCachedPlanActivity::toBSON ( BSONObjBuilder &builder ) const
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

   /*
      _optCachedPlanMonitor implement
    */
   _optCachedPlanMonitor::_optCachedPlanMonitor ()
   : _freeIndexBegin( 0 ),
     _freeIndexEnd( 0 ),
     _pFreeActivityIDs( NULL ),
     _clearThread( 0 ),
     _allocateThread( 0 ),
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

      _pFreeActivityIDs = new( std::nothrow ) UINT32[ activityNum ] ;
      if ( NULL == _pFreeActivityIDs )
      {
         goto error ;
      }

      _pActivities = new( std::nothrow ) optCachedPlanActivity[ activityNum ] ;
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
      _allocateThread.init( 0 ) ;
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
         delete [] _pFreeActivityIDs ;
      }
      if ( NULL != _pActivities )
      {
         delete [] _pActivities ;
      }
      _pFreeActivityIDs = NULL ;
      _pActivities = NULL ;
      _activityNum = 0 ;
      _highWaterMark = 0 ;
      _lowWaterMark = 0 ;
      _freeIndexBegin.init( 0 ) ;
      _freeIndexEnd.init( 0 ) ;
      _clearThread.init( 0 ) ;
      _allocateThread.init( 0 ) ;
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
      BOOLEAN criticalMode = FALSE ;

      if ( _freeIndexEnd.peek() >= OPT_PLAN_CACHE_UINT64_LIMIT )
      {
         criticalMode = TRUE ;
      }
      else if ( _cachedPlanCount.peek() > _highWaterMark )
      {
         signalPlanClearJob() ;
         criticalMode = TRUE ;
      }

      activityID = _allocateActivity( pPlan, criticalMode ) ;

      if ( OPT_INVALID_ACT_ID == activityID )
      {
         goto error ;
      }

      result = TRUE ;

   done :
      PD_TRACE_EXIT( SDB_OPTCPMON_SETACT ) ;
      return result ;

   error :
      result = FALSE ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCPMON_SIGNALJOB, "_optCachedPlanMonitor::signalPlanClearJob" )
   void _optCachedPlanMonitor::signalPlanClearJob ()
   {
      PD_TRACE_ENTRY( SDB_OPTCPMON_SIGNALJOB ) ;
      _clearEvent.signal() ;
      PD_TRACE_EXIT( SDB_OPTCPMON_SIGNALJOB ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCPMON_CHKFREEIDX, "_optCachedPlanMonitor::checkFreeIndexes" )
   void _optCachedPlanMonitor::checkFreeIndexes ()
   {
      PD_TRACE_ENTRY( SDB_OPTCPMON_CHKFREEIDX ) ;

      if ( _freeIndexEnd.peek() >= OPT_PLAN_CACHE_UINT64_LIMIT &&
           _clearThread.compareAndSwap( 0, 1 ) )
      {

         ossScopedRWLock scopedLock( &_clearLock, EXCLUSIVE ) ;

         while ( !_allocateThread.compareAndSwap( 0, 1 ) )
         {
            ossSleep( 100 ) ;
         }

         PD_LOG( PDDEBUG, "Cached Plan Monitor: free index is too large "
                 "[ %llu - %llu ], need reset", _freeIndexBegin.peek(),
                 _freeIndexEnd.peek() ) ;

         UINT64 newFreeIndexEnd = _freeIndexEnd.peek() % _activityNum ;
         _freeIndexEnd.init( newFreeIndexEnd ) ;


         UINT64 oldFreeIndexBegin = _freeIndexBegin.peek() ;
         UINT64 tmpFreeIndexBegin = oldFreeIndexBegin ;
         UINT64 newFreeIndexBegin = 0 ;
         while ( TRUE )
         {
            newFreeIndexBegin = oldFreeIndexBegin % _activityNum ;
            tmpFreeIndexBegin = _freeIndexBegin.compareAndSwapWithReturn(
                                    oldFreeIndexBegin, newFreeIndexBegin ) ;
            if ( tmpFreeIndexBegin == oldFreeIndexBegin )
            {
               break ;
            }
            oldFreeIndexBegin = tmpFreeIndexBegin ;
         }

         if ( newFreeIndexBegin > newFreeIndexEnd )
         {
            _freeIndexEnd.init( newFreeIndexEnd + _activityNum ) ;
         }

         PD_LOG( PDDEBUG, "Cached Plan Monitor: free index reseted "
                 "[ %llu - %llu ]", _freeIndexBegin.peek(),
                 _freeIndexEnd.peek() ) ;

         _allocateThread.init( 0 ) ;
         _clearThread.init( 0 ) ;
      }

      PD_TRACE_EXIT( SDB_OPTCPMON_CHKFREEIDX ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCPMON_CHKACTIME, "_optCachedPlanMonitor::checkAccessTimestamp" )
   void _optCachedPlanMonitor::checkAccessTimestamp ()
   {
      PD_TRACE_ENTRY( SDB_OPTCPMON_CHKACTIME ) ;

      if ( _accessTimestamp.peek() > OPT_PLAN_CACHE_UINT64_LIMIT )
      {
         PD_LOG( PDDEBUG, "Cached Plan Monitor: access timestamp reseted" ) ;
         _accessTimestamp.init( 0 ) ;
         _lastClearTimestamp = 0 ;
      }

      PD_TRACE_EXIT( SDB_OPTCPMON_CHKACTIME ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCPMON_CLEARCP, "_optCachedPlanMonitor::clearCachedPlans" )
   void _optCachedPlanMonitor::clearCachedPlans ()
   {
      PD_TRACE_ENTRY( SDB_OPTCPMON_CLEARCP ) ;

      if ( _clearThread.compareAndSwap( 0, 1 ) )
      {

         ossScopedRWLock scopedLock( &_clearLock, EXCLUSIVE ) ;

         UINT32 needRemoveCount = 0 ;
         double avgClearScore = 0.0 ;
         UINT64 currentTimestamp = 0 ;
         UINT64 totalAccessCount = 0 ;
         UINT64 avgAccessCount = 0 ;
         UINT32 lastClockIndex = _clockIndex ;

         UINT32 cachedPlanCount = _cachedPlanCount.peek() ;
         if ( cachedPlanCount < _highWaterMark )
         {
            _clearThread.init( 0 ) ;
            goto done ;
         }

         needRemoveCount = cachedPlanCount - _lowWaterMark ;

         PD_LOG( PDDEBUG, "Cached Plan Monitor: %u plans are cached, "
                 "%u need to be removed", cachedPlanCount, needRemoveCount ) ;

         currentTimestamp = _accessTimestamp.inc() ;

         totalAccessCount = currentTimestamp - _lastClearTimestamp ;
         totalAccessCount = OSS_MAX( 1, totalAccessCount ) ;

         avgAccessCount = totalAccessCount / cachedPlanCount ;
         avgAccessCount = OSS_MAX( 1, avgAccessCount ) ;

         avgClearScore = 1.0 / (double)cachedPlanCount ;

         while ( needRemoveCount > 0 )
         {
            optCachedPlanActivity &activity = _pActivities[ _clockIndex ] ;
            UINT64 accessTime = 0 ;
            UINT64 accessCount = 0 ;
            double curClearScore = 0.0 ;

            if ( activity.isEmpty() )
            {
               _clockIndex = ( _clockIndex + 1 ) % _activityNum ;
               if ( _clockIndex == lastClockIndex )
               {
                  break ;
               }
               continue ;
            }

            accessTime = activity.getLastAccessTime() ;
            accessCount = activity.getPeriodAccessCount() ;
            curClearScore = (double)accessCount /
                            (double)( currentTimestamp - accessTime ) ;

            if ( curClearScore > -OSS_EPSILON &&
                 curClearScore < avgClearScore )
            {
               _pPlanCache->removeCachedPlan( activity.getPlan() ) ;
               needRemoveCount -- ;
            }
            else
            {
               activity.decPeriodAccessCount( avgAccessCount ) ;
            }

            _clockIndex = ( _clockIndex + 1 ) % _activityNum ;
            if ( _clockIndex == lastClockIndex )
            {
               break ;
            }
         }

         PD_LOG( PDDEBUG, "Cached Plan Monitor: cleared %u cached plans, "
                 "%u left", cachedPlanCount - _lowWaterMark - needRemoveCount,
                 _cachedPlanCount.peek() ) ;

         _lastClearTimestamp = _accessTimestamp.inc() ;
         _clearThread.init( 0 ) ;
      }

   done :
      PD_TRACE_EXIT( SDB_OPTCPMON_CLEARCP ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTCPMON__ALLOCACT, "_optCachedPlanMonitor::_allocateActivity" )
   INT32 _optCachedPlanMonitor::_allocateActivity ( optAccessPlan *pPlan,
                                                    BOOLEAN criticalMode )
   {
      INT32 activityID = OPT_INVALID_ACT_ID ;

      PD_TRACE_ENTRY( SDB_OPTCPMON__ALLOCACT ) ;

      criticalMode |= _clearThread.compare( 1 ) ;

      if ( !criticalMode ||
           _allocateThread.compareAndSwap( 0, 1 ) )
      {
         if ( criticalMode &&
              _freeIndexEnd.compare( _freeIndexBegin.peek() ) )
         {
            _allocateThread.init( 0 ) ;
            goto done ;
         }

         UINT64 freeActivityIndex = _freeIndexBegin.inc() ;

         if ( criticalMode )
         {
            _allocateThread.init( 0 ) ;
         }

         if ( criticalMode ||
              freeActivityIndex < _freeIndexEnd.peek() )
         {
            activityID = _pFreeActivityIDs[ freeActivityIndex % _activityNum ] ;
            optCachedPlanActivity &activity = _pActivities[ activityID ] ;

            SDB_ASSERT( activity.isEmpty(), "Activity is not empty" ) ;

            pPlan->setActivityID( activityID ) ;
            activity.setPlan( pPlan, _accessTimestamp.inc() ) ;
            activity.incPeriodAccessCount() ;
         }
         else
         {



         }
      }

   done :
      PD_TRACE_EXIT( SDB_OPTCPMON__ALLOCACT ) ;
      return activityID ;
   }

   /*
      _optAccessPlanManager implement
    */
   _optAccessPlanManager::_optAccessPlanManager ()
   : _optAccessPlanConfigHolder(),
     _mthMatchConfigHolder(),
     _planCache(),
     _monitor(),
     _clearJobEduID( PMD_INVALID_EDUID ),
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
                                       BOOLEAN enableMixCmp )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_INIT ) ;

      SDB_ASSERT( !_planCache.isInitialized(),
                  "cache should not be initialized" ) ;

      _cacheLevel = OPT_PLAN_NOCACHE ;

      setSortBufferSize( sortBufferSize ) ;
      setOptCostThreshold( optCostThreshold ) ;

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

         rc = _startClearJob() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start cached-plan clearing job "
                      "failed, rc: %d", rc ) ;
      }
      else
      {
         bucketNum = 0 ;
         cacheLevel = OPT_PLAN_NOCACHE ;
      }

      sdbGetDMSCB()->changeSUCaches( getMask() ) ;

      setMthEnableParameterized( cacheLevel >= OPT_PLAN_PARAMETERIZED ) ;
      setMthEnableFuzzyOptr( cacheLevel >= OPT_PLAN_FUZZYOPTR ) ;

      _cacheLevel = cacheLevel ;

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
                                         BOOLEAN enableMixCmp )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_REINIT ) ;

      ossScopedLock scopedLock( &_reinitLatch ) ;

      if ( _planCache.isInitialized() )
      {
         bucketNum = _planCache.getRoundedUpBucketNum( bucketNum ) ;

         if ( bucketNum != _planCache.getBucketNum() ||
              cacheLevel != _cacheLevel ||
              sortBufferSize != getSortBufferSize() ||
              optCostThreshold != getOptCostThreshold() ||
              enableMixCmp != mthEnabledMixCmp() )
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
               if ( 0 == bucketNum )
               {
                  _stopClearJob() ;
               }

               rc = fini() ;
               PD_RC_CHECK( rc, PDERROR, "Failed to finalize access plan "
                            "manager, rc: %d", rc ) ;

               rc = init( bucketNum, cacheLevel, sortBufferSize,
                          optCostThreshold, enableMixCmp ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to initialize access plan "
                            "manager, rc: %d", rc ) ;
            }
         }
      }
      else
      {
         rc = init( bucketNum, cacheLevel, sortBufferSize,
                    optCostThreshold, enableMixCmp ) ;
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

         _planCache.invalidateAllPlans() ;

         _planCache.deinitialize() ;
         _monitor.deinitialize() ;
      }

      PD_TRACE_EXIT( SDB_OPTAPM_FINI ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_GETAP, "_optAccessPlanManager::getAccessPlan" )
   INT32 _optAccessPlanManager::getAccessPlan ( const rtnQueryOptions &options,
                                                BOOLEAN keepSearchPaths,
                                                dmsStorageUnit *su,
                                                dmsMBContext *mbContext,
                                                optAccessPlanRuntime &planRuntime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_GETAP ) ;

      SDB_ASSERT( su, "su is invalid" ) ;
      SDB_ASSERT( mbContext, "mbContext is invalid" ) ;

      BOOLEAN gotMainCLPlan = FALSE ;

      if ( isInitialized() &&
           _cacheLevel >= OPT_PLAN_PARAMETERIZED &&
           NULL != options.getMainCLName() &&
           !keepSearchPaths )
      {
         dmsCachedPlanMgr *pCachedPlanMgr = su->getCachedPlanMgr() ;
         if ( NULL == pCachedPlanMgr ||
              pCachedPlanMgr->testMainCLInvalidBitmap( mbContext->mbID() ) )
         {
            gotMainCLPlan = FALSE ;
         }
         else
         {
            rc = _getMainCLAccessPlan( options, su, mbContext, planRuntime ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get main-collection access plan "
                         "for query [ %s ], rc: %d", options.toString().c_str(),
                         rc ) ;
            gotMainCLPlan = TRUE ;
         }
      }

      if ( !gotMainCLPlan )
      {
         rc = _getCLAccessPlan( options, keepSearchPaths, su, mbContext,
                                planRuntime ) ;
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

      rc = _getCLAccessPlan( options, OPT_PLAN_NOCACHE, FALSE, su, mbContext,
                             planRuntime ) ;
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
         _planCache.invalidateAllPlans() ;
      }

      PD_TRACE_EXIT( SDB_OPTAPM_INVALIDALLPLANS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_SETQUERYACT, "_optAccessPlanManager::setQueryActivity" )
   void _optAccessPlanManager::setQueryActivity ( INT32 activityID,
                                                  const optQueryActivity &queryActivity )
   {
      PD_TRACE_ENTRY( SDB_OPTAPM_SETQUERYACT ) ;

      if ( isInitialized() )
      {
         optCachedPlanActivity *activity = _monitor.getActivity( activityID ) ;
         if ( NULL != activity )
         {
            activity->setQueryActivity( queryActivity ) ;
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

         invalidateSUPlans( pOldCSName ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTAPM_ONRENAMECS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_ONDROPCS, "_optAccessPlanManager::onDropCS" )
   INT32 _optAccessPlanManager::onDropCS ( IDmsEventHolder *pEventHolder,
                                           IDmsSUCacheHolder *pCacheHolder,
                                           pmdEDUCB *cb,
                                           SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_ONDROPCS ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      if ( pCacheHolder && isInitialized() )
      {
         _invalidSUPlans( pCacheHolder ) ;

         invalidateSUPlans( pCacheHolder->getCSName() ) ;
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
   INT32 _optAccessPlanManager::onTruncateCL ( IDmsEventHolder *pEventHolder,
                                               IDmsSUCacheHolder *pCacheHolder,
                                               const dmsEventCLItem &clItem,
                                               UINT32 newCLLID,
                                               pmdEDUCB *cb,
                                               SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_ONTRUNCATECL ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      if ( pCacheHolder && isInitialized() )
      {
         _invalidCLPlans( pCacheHolder, clItem._mbID, clItem._clLID ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTAPM_ONTRUNCATECL, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_ONDROPCL, "_optAccessPlanManager::onDropCL" )
   INT32 _optAccessPlanManager::onDropCL ( IDmsEventHolder *pEventHolder,
                                           IDmsSUCacheHolder *pCacheHolder,
                                           const dmsEventCLItem &clItem,
                                           pmdEDUCB *cb,
                                           SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_ONDROPCL ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      if ( pCacheHolder && isInitialized() )
      {
         _invalidCLPlans( pCacheHolder, clItem._mbID, clItem._clLID ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTAPM_ONDROPCL, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM_ONCRTIDX, "_optAccessPlanManager::onCreateIndex" )
   INT32 _optAccessPlanManager::onCreateIndex ( IDmsEventHolder *pEventHolder,
                                                IDmsSUCacheHolder *pCacheHolder,
                                                const dmsEventCLItem &clItem,
                                                const dmsEventIdxItem &idxItem,
                                                pmdEDUCB *cb,
                                                SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM_ONCRTIDX ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      if ( pCacheHolder && isInitialized() )
      {
         _invalidCLPlans( pCacheHolder, clItem._mbID, clItem._clLID ) ;
      }

      PD_TRACE_EXITRC( SDB_OPTAPM_ONCRTIDX, rc ) ;

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
                                                   BOOLEAN keepSearchPaths,
                                                   dmsStorageUnit *su,
                                                   dmsMBContext *mbContext,
                                                   optAccessPlanRuntime &planRuntime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM__GETCLAP ) ;

      SDB_ASSERT( su, "su is invalid" ) ;
      SDB_ASSERT( mbContext, "mbContext is invalid" ) ;

      OPT_PLAN_CACHE_LEVEL cacheLevel = _cacheLevel ;

      if ( keepSearchPaths )
      {
         cacheLevel = OPT_PLAN_NOCACHE ;
      }

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

      rc = _getCLAccessPlan( options, cacheLevel, keepSearchPaths, su,
                             mbContext, planRuntime ) ;
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
                                                   BOOLEAN keepSearchPaths,
                                                   dmsStorageUnit *su,
                                                   dmsMBContext *mbContext,
                                                   optAccessPlanRuntime &planRuntime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPM__GETCLAP_LEVEL ) ;

      SDB_ASSERT( su, "su is invalid" ) ;
      SDB_ASSERT( mbContext, "mbContext is invalid" ) ;

      optGeneralAccessPlan *pPlan = NULL ;
      optAccessPlan *pTmpPlan = NULL ;

      optAccessPlanKey planKey( options, cacheLevel ) ;

      optAccessPlanHelper planHelper( cacheLevel, getPlanConfig(),
                                      getMatchConfig(), keepSearchPaths ) ;
      BOOLEAN needCache = ( isInitialized() &&
                            cacheLevel > OPT_PLAN_NOCACHE &&
                            !keepSearchPaths ) ;

      planRuntime.reset() ;

      rc = _prepareAccessPlanKey( su, mbContext, planKey, planHelper,
                                  planRuntime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare key of access plan, rc: %d",
                   rc ) ;

      if ( needCache )
      {
         rc = _getCachedAccessPlan( planKey, &pTmpPlan ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get access plan, rc: %d", rc ) ;
         if ( NULL != pTmpPlan )
         {
            pPlan = dynamic_cast<_optGeneralAccessPlan *>( pTmpPlan ) ;
            if ( NULL == pPlan )
            {
               pTmpPlan->release() ;
            }
         }
      }

      if ( NULL == pPlan )
      {
         rc = _createAccessPlan( su, mbContext, planKey, planRuntime,
                                 planHelper, &pPlan, needCache ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create access plan, rc: %d",
                      rc ) ;

         planRuntime.setPlan( pPlan, this, TRUE ) ;
      }
      else
      {
         if ( pPlan->getCacheLevel() >= OPT_PLAN_PARAMETERIZED )
         {
            optParamAccessPlan *paramPlan = dynamic_cast<optParamAccessPlan *>( pPlan ) ;
            SDB_ASSERT( paramPlan, "paramPlan is invalid" ) ;

            rc = _validateParamPlan( su, mbContext, planKey, planRuntime,
                                     planHelper, paramPlan ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to validate parameterized plan, "
                         "rc: %d", rc ) ;
         }
         else
         {
            planRuntime.deleteMatchRuntime() ;
         }

         if ( planRuntime.hasPlan() )
         {
            pPlan->release() ;
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
         rc = _getCLAccessPlan( options, FALSE, su, mbContext, planRuntime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get collection access plan for "
                      "query [ %s ], rc: %d", options.toString().c_str(), rc ) ;
      }
      else
      {
         optMainCLAccessPlan *mainPlan = NULL ;

         optAccessPlanKey planKey( options, cacheLevel ) ;
         planKey.setCLFullName( options.getMainCLName() ) ;
         planKey.setMainCLName( NULL ) ;

         optAccessPlanHelper planHelper( cacheLevel, getPlanConfig(),
                                         getMatchConfig(), FALSE ) ;

         rc = _prepareAccessPlanKey( NULL, NULL, planKey, planHelper,
                                     planRuntime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to prepare key of access plan, "
                      "rc: %d", rc ) ;

         rc = _getCachedAccessPlan( planKey, &pPlan ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get access plan, rc: %d", rc ) ;

         if ( NULL == pPlan )
         {
            rc = _createMainCLPlan( planKey, options, su, mbContext,
                                    planRuntime, planHelper, &mainPlan ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to create main-collection "
                         "query, rc: %d", rc ) ;

            _cacheAccessPlan( mainPlan ) ;

            mainPlan->release() ;
            pPlan = NULL ;
         }
         else
         {
            mainPlan = dynamic_cast<optMainCLAccessPlan *>( pPlan ) ;
            SDB_ASSERT( mainPlan, "mainPlan is invalid " ) ;

            if ( pCachedPlanMgr->testParamInvalidBitmap( subCLMBID ) ||
                 !mainPlan->isMainCLValid() )
            {
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
               pPlan->release() ;
            }
            else
            {
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
            planRuntime.deleteMatchRuntime() ;
         }
      }

      planKey.generateKeyCode( su, mbContext ) ;

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
         BSONObj parameters = planRuntime.getParameters().toBSON() ;
         pPlan->validateParameterized( *pPlan, parameters ) ;
      }

      (*ppPlan) = pPlan ;

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

      _monitor.incCachedPlanCount() ;

      cached = _planCache.addPlan( pPlan ) ;
      if ( cached )
      {
         if ( !_monitor.setActivity( pPlan ) )
         {
            if ( _planCache.removeItem( pPlan ) )
            {
               cached = FALSE ;
               _monitor.decCachedPlanCount( 1 ) ;
            }
         }
         else
         {
            if ( !pPlan->isCached() )
            {
               _monitor.resetActivity( pPlan->resetActivityID() ) ;
               cached = FALSE ;
            }
         }
      }
      else
      {
         _monitor.decCachedPlanCount( 1 ) ;
      }

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

      if ( plan->isParamValid() )
      {
         rc = planRuntime.bindParamPlan( planHelper, plan ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to bind parameterized plan, rc: %d",
                      rc ) ;
         goto done ;
      }

      parameters = planRuntime.getParameters().toBSON() ;
      if ( plan->checkSavedParam( parameters ) )
      {
         rc = planRuntime.bindParamPlan( planHelper, plan ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to bind parameterized plan, rc: %d",
                      rc ) ;
         goto done ;
      }

      rc = _getCLAccessPlan( planKey, OPT_PLAN_NOCACHE, FALSE, su, mbContext,
                             planRuntime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get access plan for with "
                   "query [ %s ], rc: %d", planKey.toString().c_str(), rc ) ;

      tempPlan = dynamic_cast<optGeneralAccessPlan *>( planRuntime.getPlan() ) ;
      SDB_ASSERT( tempPlan, "subPlan is invalid " ) ;

      if ( plan->validateParameterized( *tempPlan, parameters ) )
      {
      }
      else
      {
         PD_LOG( PDDEBUG, "Invalid parameterized plan [%s]",
                 plan->toString().c_str() ) ;
         plan->markParamInvalid( mbContext ) ;
         _planCache.removeCachedPlan( plan ) ;
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

      rc = mainPlan->getKeyOwned() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key of access plan owned, "
                   "rc: %d", rc ) ;

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

      if ( !planRuntime.getParameters().isEmpty() )
      {
         parameters = planRuntime.getParameters().toBSON() ;
      }

      rc = mainPlan->prepareBindSubCL( planHelper ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare main-collection "
                   "plan, rc: %d", rc ) ;

      rc = _getCLAccessPlan( subOptions, OPT_PLAN_NOCACHE, FALSE, su, mbContext,
                             planRuntime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get access plan for "
                   "sub-collection with query [ %s ], rc: %d",
                   subOptions.toString().c_str(), rc ) ;

      subPlan = dynamic_cast<optGeneralAccessPlan *>( planRuntime.getPlan() ) ;
      SDB_ASSERT( subPlan, "subPlan is invalid " ) ;

      rc = mainPlan->bindSubCLAccessPlan( planHelper, subPlan, parameters ) ;
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

      optGeneralAccessPlan *subPlan = NULL ;
      dmsCachedPlanMgr *pCachedPlanMgr = su->getCachedPlanMgr() ;
      BSONObj parameters ;

      if ( mainPlan->getCacheLevel() >= OPT_PLAN_PARAMETERIZED &&
           !planRuntime.getParameters().isEmpty() )
      {
         parameters = planRuntime.getParameters().toBSON() ;
      }

      if ( mainPlan->checkSavedSubCL( subOptions.getCLFullName(),
                                      parameters ) )
      {
         rc = _bindMainCLPlan( mainPlan, subOptions, su, mbContext,
                               planRuntime, planHelper ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to bind main-collection "
                      "plan, rc: %d", rc ) ;
         goto done ;
      }

      rc = _getCLAccessPlan( subOptions, OPT_PLAN_NOCACHE, FALSE, su, mbContext,
                             planRuntime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get access plan for "
                   "sub-collection with query [ %s ], rc: %d",
                   subOptions.toString().c_str(), rc ) ;

      subPlan = dynamic_cast<optGeneralAccessPlan *>( planRuntime.getPlan() ) ;
      SDB_ASSERT( subPlan, "subPlan is invalid " ) ;

      if ( !mainPlan->validateSubCLPlan( subPlan, parameters ) )
      {
         PD_LOG( PDDEBUG, "Invalid main-collection plan [%s]",
                 mainPlan->toString().c_str() ) ;
         mainPlan->markMainCLInvalid( pCachedPlanMgr, mbContext, FALSE ) ;
         _planCache.removeCachedPlan( mainPlan ) ;
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

      rc = mainPlan->validateSubCL( su, mbContext, indexExtID, indexLID ) ;
      if ( SDB_OK != rc )
      {
         rc = mainPlan->markMainCLInvalid( pCachedPlanMgr,
                                           mbContext,
                                           TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to mark sub-collection "
                      "invalidated to reuse main-collection plan, "
                      "rc: %d", rc ) ;

         rc = _getCLAccessPlan( subOptions, FALSE, su, mbContext, planRuntime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get collection access plan for "
                      "query [ %s ], rc: %d", subOptions.toString().c_str(),
                      rc ) ;

         goto done ;
      }

      rc = planRuntime.bindPlanInfo( subOptions.getCLFullName(), su, mbContext,
                                     indexExtID, indexLID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to bind plan info, rc: %d",
                   rc ) ;

      if ( mainPlan->getCacheLevel() >= OPT_PLAN_PARAMETERIZED )
      {
         rc = planRuntime.bindParamPlan( planHelper, mainPlan ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to bind parameterized "
                      "plan, rc: %d", rc ) ;
      }
      else
      {
         planRuntime.deleteMatchRuntime() ;
      }

   done :
      PD_TRACE_EXITRC( SDB_OPTAPM__BINDMAINCLPLAN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPM__INVALIDPLANS, "_optAccessPlanManager::_invalidCachedPlans" )
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
