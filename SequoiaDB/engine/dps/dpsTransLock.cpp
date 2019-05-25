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

   Source File Name = dpsTransLock.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "dpsTransLock.hpp"
#include "dpsTransLockBucket.hpp"
#include "pd.hpp"
#include "dpsTransLockDef.hpp"
#include "dms.hpp"
#include "pmdEDU.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"

namespace engine
{
   dpsTransLock::dpsTransLock()
   :_bucketLst(MAX_LOCKBUCKET_NUM, (dpsLockBucket*)NULL)
   {
   }

   dpsTransLock::~dpsTransLock()
   {
      UINT32 i = 0;
      for ( ; i < MAX_LOCKBUCKET_NUM; i++ )
      {
         if ( _bucketLst[i] != NULL )
         {
            SDB_OSS_DEL( _bucketLst[i] );
         }
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_ACQUIREX, "dpsTransLock::acquireX" )
   INT32 dpsTransLock::acquireX( _pmdEDUCB *eduCB,
                                 const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_ACQUIREX ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;
      dpsTransLockId iLockId;
      dpsTransCBLockInfo *pLockInfo = NULL;
      BOOLEAN isIXLocked = FALSE;

      if ( lockId._collectionID != DMS_INVALID_MBID )
      {
         iLockId = lockId ;
         if ( lockId._recordOffset != DMS_INVALID_OFFSET )
         {
            iLockId._recordExtentID = DMS_INVALID_EXTENT;
            iLockId._recordOffset = DMS_INVALID_OFFSET;
         }
         else
         {
            iLockId._collectionID = DMS_INVALID_MBID;
         }
         rc = acquireIX( eduCB, iLockId);
   
         PD_RC_CHECK( rc, PDERROR,
                     "failed to get the intention-lock, "
                     "get X-lock failed(rc=%d)",
                     rc );
         isIXLocked = TRUE;
      }

      pLockInfo = eduCB->getTransLock( lockId );
      if ( pLockInfo != NULL && pLockInfo->isLockMatch( DPS_TRANSLOCK_X ))
      {
         pLockInfo->incRef();
         goto done;
      }

      if ( pLockInfo )
      {
         rc = upgrade( eduCB, lockId, pLockInfo, DPS_TRANSLOCK_X );
         if ( SDB_OK == rc )
         {
            pLockInfo->incRef();
         }
      }
      else
      {
         rc = getBucket( lockId, pLockBucket );
         PD_RC_CHECK( rc, PDERROR, "Failed to get the lock-bucket, "
                      "get X-lock failed(rc=%d)", rc );
         {
         DPS_TRANS_WAIT_LOCK _transWaitLock( eduCB, lockId ) ;
         eduCB->addLockInfo( lockId, DPS_TRANSLOCK_X );
         rc = pLockBucket->acquire( eduCB, lockId, DPS_TRANSLOCK_X );
         }
         if ( rc )
         {
            eduCB->delLockInfo( lockId );
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get the X-lock(rc=%d)", rc );

      PD_LOG( PDDEBUG, "Get the X-lock(%s)", lockId.toString().c_str() );

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_ACQUIREX );
      return rc;
   error:
      if ( isIXLocked )
      {
         release( eduCB, iLockId );
      }
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_ACQUIRES, "dpsTransLock::acquireS" )
   INT32 dpsTransLock::acquireS( _pmdEDUCB *eduCB,
                                 const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_ACQUIRES ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;
      dpsTransLockId iLockId;
      dpsTransCBLockInfo *pLockInfo = NULL;
      BOOLEAN isISLocked = FALSE;

      if ( lockId._collectionID != DMS_INVALID_MBID )
      {
         iLockId = lockId ;
         if ( lockId._recordOffset != DMS_INVALID_OFFSET )
         {
            iLockId._recordExtentID = DMS_INVALID_EXTENT;
            iLockId._recordOffset = DMS_INVALID_OFFSET;
         }
         else
         {
            iLockId._collectionID = DMS_INVALID_MBID;
         }
         rc = acquireIS( eduCB, iLockId);
   
         PD_RC_CHECK( rc, PDERROR, "Failed to get the intention-lock, "
                      "get S-lock failed(rc=%d)", rc );
         isISLocked = TRUE;
      }

      pLockInfo = eduCB->getTransLock( lockId );
      if ( pLockInfo != NULL && pLockInfo->isLockMatch( DPS_TRANSLOCK_S ))
      {
         pLockInfo->incRef();
         goto done;
      }

      if ( pLockInfo )
      {
         rc = upgrade( eduCB, lockId, pLockInfo, DPS_TRANSLOCK_S );
         if ( SDB_OK == rc )
         {
            pLockInfo->incRef();
         }
      }
      else
      {
         rc = getBucket( lockId, pLockBucket );
         PD_RC_CHECK( rc, PDERROR, "Failed to get the lock-bucket, "
                      "get S-lock failed(rc=%d)", rc );
         {
         DPS_TRANS_WAIT_LOCK _transWaitLock( eduCB, lockId ) ;
         eduCB->addLockInfo( lockId, DPS_TRANSLOCK_S);
         rc = pLockBucket->acquire( eduCB, lockId, DPS_TRANSLOCK_S );
         }
         if ( rc )
         {
            eduCB->delLockInfo( lockId );
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get the S-lock(rc=%d)", rc );

      PD_LOG( PDDEBUG, "Get the S-lock(%s)", lockId.toString().c_str() );

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_ACQUIRES );
      return rc;
   error:
      if ( isISLocked )
      {
         release( eduCB, iLockId );
      }
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_ACQUIREIX, "dpsTransLock::acquireIX" )
   INT32 dpsTransLock::acquireIX( _pmdEDUCB *eduCB,
                                  const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_ACQUIREIX ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      SDB_ASSERT( ( DMS_INVALID_OFFSET == lockId._recordOffset
                  && DMS_INVALID_EXTENT == lockId._recordExtentID ),
                  "IX-Lock only used for collection or collectionspace" ) ;

      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;
      dpsTransLockId isLockId;
      dpsTransCBLockInfo *pLockInfo = NULL;
      BOOLEAN isISLocked = FALSE;

      if ( lockId._collectionID != DMS_INVALID_MBID )
      {
         isLockId._logicCSID = lockId._logicCSID;
         rc = acquireIS( eduCB, isLockId );
         PD_RC_CHECK( rc, PDERROR, "Failed to get the S-lock of space, "
                      "get IX-lock failed(rc=%d)", rc );
         isISLocked = TRUE;
      }

      pLockInfo = eduCB->getTransLock( lockId );
      if ( pLockInfo != NULL && pLockInfo->isLockMatch( DPS_TRANSLOCK_IX ))
      {
         pLockInfo->incRef();
         goto done;
      }

      if ( pLockInfo )
      {
         rc = upgrade( eduCB, lockId, pLockInfo, DPS_TRANSLOCK_IX );
         if ( SDB_OK == rc )
         {
            pLockInfo->incRef();
         }
      }
      else
      {
         rc = getBucket( lockId, pLockBucket );
         PD_RC_CHECK( rc, PDERROR, "Failed to get the lock-bucket, "
                      "get IX-lock failed(rc=%d)", rc );

         {
         DPS_TRANS_WAIT_LOCK _transWaitLock( eduCB, lockId ) ;
         eduCB->addLockInfo( lockId, DPS_TRANSLOCK_IX );
         rc = pLockBucket->acquire( eduCB, lockId, DPS_TRANSLOCK_IX );
         }
         if ( rc )
         {
            eduCB->delLockInfo( lockId );
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get the IX-lock(rc=%d)", rc );

      PD_LOG( PDDEBUG, "Get the IX-lock(%s)", lockId.toString().c_str() );

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_ACQUIREIX );
      return rc;
   error:
      if ( isISLocked )
      {
         release( eduCB, isLockId );
      }
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_ACQUIREIS, "dpsTransLock::acquireIS" )
   INT32 dpsTransLock::acquireIS( _pmdEDUCB *eduCB,
                                  const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_ACQUIREIS ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      SDB_ASSERT( ( DMS_INVALID_OFFSET == lockId._recordOffset
                  && DMS_INVALID_EXTENT == lockId._recordExtentID ),
                  "IX-Lock only used for collection or collectionspace" ) ;

      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;
      dpsTransLockId sLockId;
      dpsTransCBLockInfo *pLockInfo = NULL;
      BOOLEAN isISLocked = FALSE;

      if ( lockId._collectionID != DMS_INVALID_MBID )
      {
         sLockId._logicCSID = lockId._logicCSID;
         rc = acquireIS( eduCB, sLockId );
         PD_RC_CHECK( rc, PDERROR, "Failed to get the S-lock of space, "
                      "get IS-lock failed(rc=%d)", rc );
         isISLocked = TRUE;
      }

      pLockInfo = eduCB->getTransLock( lockId );
      if ( pLockInfo != NULL )
      {
         pLockInfo->incRef();
         goto done;
      }

      rc = getBucket( lockId, pLockBucket );
      PD_RC_CHECK( rc, PDERROR, "Failed to get the lock-bucket, "
                   "get IS-lock failed(rc=%d)", rc );

      {
      DPS_TRANS_WAIT_LOCK _transWaitLock( eduCB, lockId ) ;
      eduCB->addLockInfo( lockId, DPS_TRANSLOCK_IS );
      rc = pLockBucket->acquire( eduCB, lockId, DPS_TRANSLOCK_IS );
      }
      if ( rc )
      {
         eduCB->delLockInfo( lockId );
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get the IS-lock(rc=%d)", rc );

      PD_LOG( PDDEBUG, "Get the IS-lock(%s)", lockId.toString().c_str() );

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_ACQUIREIS );
      return rc;
   error:
      if ( isISLocked )
      {
         release( eduCB, sLockId );
      }
      goto done;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_RELEASE, "dpsTransLock::release" )
   void dpsTransLock::release( _pmdEDUCB * eduCB,
                               const dpsTransLockId & lockId )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_RELEASE ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;
      dpsTransLockId iLockId;
      dpsTransCBLockInfo *pLockInfo = NULL;
      INT64 lockRef = 0;

      pLockInfo = eduCB->getTransLock( lockId );
      PD_CHECK( pLockInfo, SDB_OK, done, PDWARNING,
                "duplicate release lock(%s)",
                lockId.toString().c_str() ) ;
      lockRef = pLockInfo->decRef();
      if ( lockRef <= 0 )
      {
         rc = getBucket( lockId, pLockBucket );
         if ( rc )
         {
            PD_LOG( PDWARNING, "Failed to get lock-bucket while release "
                    "lock(rc=%d)", rc );
         }
         else
         {
            pLockBucket->release( eduCB, lockId );
         }

         eduCB->delLockInfo( lockId );
         pLockInfo = NULL;
      }

      if ( lockId._collectionID != DMS_INVALID_MBID )
      {
         iLockId = lockId;
         if ( lockId._recordOffset != DMS_INVALID_OFFSET )
         {
            iLockId._recordExtentID = DMS_INVALID_EXTENT;
            iLockId._recordOffset = DMS_INVALID_OFFSET;
            release( eduCB, iLockId );
         }
         else
         {
            iLockId._collectionID = DMS_INVALID_MBID;
            release( eduCB, iLockId );
         }
      }

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_RELEASE );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_RELEASEALL, "dpsTransLock::releaseAll" )
   void dpsTransLock::releaseAll( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_RELEASEALL ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;
      DpsTransCBLockList *pLockLst = eduCB->getLockList();
      DpsTransCBLockList::iterator iterLst = pLockLst->begin();
      while ( iterLst != pLockLst->end() )
      {
         rc = getBucket( iterLst->first, pLockBucket );
         if ( rc )
         {
            PD_LOG( PDWARNING, "Failed to get lock-bucket while release "
                    "lock(rc=%d)", rc );
         }
         else
         {
            pLockBucket->release( eduCB, iterLst->first );
         }
         iterLst++ ;
      }
      eduCB->clearLockList() ;
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_RELEASEALL );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_UPGRADE, "dpsTransLock::upgrade" )
   INT32 dpsTransLock::upgrade( _pmdEDUCB *eduCB,
                                const dpsTransLockId &lockId,
                                dpsTransCBLockInfo *pLockInfo,
                                DPS_TRANSLOCK_TYPE lockType )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_UPGRADE ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      INT32 rc = SDB_OK;
      DPS_TRANSLOCK_TYPE lastLockType;
      dpsLockBucket *pLockBucket = NULL;

      rc = upgradeCheck( pLockInfo->getType(), lockType );
      PD_RC_CHECK( rc, PDERROR, "Upgrade lock failed(rc=%d)", rc );

      rc = getBucket( lockId, pLockBucket );
      PD_RC_CHECK( rc, PDERROR, "Failed to get lock-bucket, upgrade lock "
                   "failed(rc=%d)", rc );

      lastLockType = pLockInfo->getType();
      pLockInfo->setType( lockType );
      rc = pLockBucket->upgrade( eduCB, lockId, lockType );
      PD_CHECK( SDB_OK == rc, rc, rollbackType, PDERROR,
                "upgrade lock failed(rc=%d)", rc );

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_UPGRADE );
      return rc;
   rollbackType:
      pLockInfo->setType( lastLockType );
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_UPGRADECHECK, "dpsTransLock::upgradeCheck" )
   INT32 dpsTransLock::upgradeCheck( DPS_TRANSLOCK_TYPE srcType,
                                     DPS_TRANSLOCK_TYPE dstType )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_UPGRADECHECK ) ;
      if ( (dstType - srcType == 1 && srcType != DPS_TRANSLOCK_IX ) ||
           (dstType - srcType == 2 && dstType != DPS_TRANSLOCK_X ) )
      {
         return SDB_OK;
      }
      PD_LOG( PDERROR, "Couldn't upgrade from(%d) to (%d)", srcType, dstType ) ;
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_UPGRADECHECK );
      return SDB_PERM;
   }

   UINT32 dpsTransLock::getBucketNo( const dpsTransLockId &lockId )
   {
      if ( lockId._recordOffset != DMS_INVALID_OFFSET )
      {
         return lockId._recordOffset % MAX_LOCKBUCKET_NUM ;
      }
      else if ( lockId._collectionID != DMS_INVALID_MBID )
      {
         return lockId._collectionID % MAX_LOCKBUCKET_NUM;
      }
      return lockId._logicCSID % MAX_LOCKBUCKET_NUM;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_GETBUCKET, "dpsTransLock::getBucket" )
   INT32 dpsTransLock::getBucket( const dpsTransLockId &lockId,
                                  dpsLockBucket *&lockBucket )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_GETBUCKET ) ;
      INT32 rc = SDB_OK;
      UINT32 bucketNo = getBucketNo( lockId );
      if ( NULL == _bucketLst[ bucketNo ] )
      {
         ossScopedLock _lock( &_LstMutex );
         if ( NULL == _bucketLst[ bucketNo ] )
         {
            dpsLockBucket *lockBucket = SDB_OSS_NEW dpsLockBucket;
            PD_CHECK( lockBucket != NULL, SDB_OOM, error, PDERROR,
                      "Failed to allocate lock-bucket, malloc error ");
            _bucketLst[ bucketNo ] = lockBucket;
         }
      }
   done:
      lockBucket = _bucketLst[ bucketNo ];
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_GETBUCKET );
      return rc;
   error:
      goto done;
   }

   //PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_TESTS, "dpsTransLock::testS" )
   INT32 dpsTransLock::testS( _pmdEDUCB *eduCB, const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_TESTS ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;
      dpsTransLockId iLockId;
      dpsTransCBLockInfo *pLockInfo = NULL;

      pLockInfo = eduCB->getTransLock( lockId );
      if ( pLockInfo != NULL && pLockInfo->isLockMatch( DPS_TRANSLOCK_S ))
      {
         goto done;
      }

      if ( lockId._collectionID != DMS_INVALID_MBID )
      {
         iLockId = lockId ;
         if ( lockId._recordOffset != DMS_INVALID_OFFSET )
         {
            iLockId._recordExtentID = DMS_INVALID_EXTENT;
            iLockId._recordOffset = DMS_INVALID_OFFSET;
         }
         else
         {
            iLockId._collectionID = DMS_INVALID_MBID;
         }
         rc = testIS( eduCB, iLockId );
   
         PD_RC_CHECK( rc, PDINFO, "Failed to test the intention-lock, "
                     "test S-lock failed(rc=%d)", rc );
      }

      if ( pLockInfo )
      {
         rc = testUpgrade( eduCB, lockId, pLockInfo, DPS_TRANSLOCK_S );
      }
      else
      {
         rc = getBucket( lockId, pLockBucket );
         PD_RC_CHECK( rc, PDERROR, "Failed to test the lock-bucket, "
                      "get lock-bucket failed(rc=%d)", rc );
         rc = pLockBucket->test( eduCB, lockId, DPS_TRANSLOCK_S );
      }
      PD_RC_CHECK( rc, PDINFO, "Failed to test the S-lock(rc=%d)", rc );

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_TESTS );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_TESTUPGRADE, "dpsTransLock::testUpgrade" )
   INT32 dpsTransLock::testUpgrade( _pmdEDUCB *eduCB,
                                    const dpsTransLockId &lockId,
                                    dpsTransCBLockInfo *pLockInfo,
                                    DPS_TRANSLOCK_TYPE lockType )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_TESTUPGRADE ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;

      rc = upgradeCheck( pLockInfo->getType(), lockType );
      PD_RC_CHECK( rc, PDERROR, "Upgrade lock failed(rc=%d)", rc );

      rc = getBucket( lockId, pLockBucket );
      PD_RC_CHECK( rc, PDERROR, "Failed to get lock-bucket, upgrade lock "
                   "failed(rc=%d)", rc );

      rc = pLockBucket->test( eduCB, lockId, lockType );
      PD_RC_CHECK( rc, PDERROR, "Upgrade lock failed(rc=%d)", rc ) ;

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_TESTUPGRADE );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_TESTIS, "dpsTransLock::testIS" )
   INT32 dpsTransLock::testIS( _pmdEDUCB *eduCB, const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_TESTIS ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      SDB_ASSERT( ( DMS_INVALID_OFFSET == lockId._recordOffset
                  && DMS_INVALID_EXTENT == lockId._recordExtentID ),
                  "IX-Lock only used for collection or collectionspace" ) ;
      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;
      dpsTransLockId sLockId;
      dpsTransCBLockInfo *pLockInfo = NULL;

      pLockInfo = eduCB->getTransLock( lockId );
      if ( pLockInfo != NULL )
      {
         goto done;
      }

      if ( lockId._collectionID != DMS_INVALID_MBID )
      {
         sLockId._logicCSID = lockId._logicCSID;
         rc = testIS( eduCB, sLockId );
         PD_RC_CHECK( rc, PDINFO, "Failed to test the S-lock of space, "
                      "test IS-lock failed(rc=%d)", rc );
      }

      rc = getBucket( lockId, pLockBucket );
      PD_RC_CHECK( rc, PDERROR, "Failed to get the lock-bucket, "
                   "test IS-lock failed(rc=%d)", rc );

      rc = pLockBucket->test( eduCB, lockId, DPS_TRANSLOCK_IS );
      PD_RC_CHECK( rc, PDINFO, "Failed to test the IS-lock(rc=%d)", rc );

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_TESTIS );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_TESTX, "dpsTransLock::testX" )
   INT32 dpsTransLock::testX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_TESTX ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;
      dpsTransLockId iLockId;
      dpsTransCBLockInfo *pLockInfo = NULL;

      pLockInfo = eduCB->getTransLock( lockId );
      if ( pLockInfo != NULL && pLockInfo->isLockMatch( DPS_TRANSLOCK_X ))
      {
         goto done;
      }

      if ( lockId._collectionID != DMS_INVALID_MBID )
      {
         iLockId = lockId ;
         if ( lockId._recordOffset != DMS_INVALID_OFFSET )
         {
            iLockId._recordExtentID = DMS_INVALID_EXTENT;
            iLockId._recordOffset = DMS_INVALID_OFFSET;
         }
         else
         {
            iLockId._collectionID = DMS_INVALID_MBID;
         }
         rc = testIX( eduCB, iLockId);
   
         PD_RC_CHECK( rc, PDINFO, "Failed to test the intention-lock, "
                      "test IX-lock failed(rc=%d)", rc );
      }

      if ( pLockInfo )
      {
         rc = testUpgrade( eduCB, lockId, pLockInfo, DPS_TRANSLOCK_X );
      }
      else
      {
         rc = getBucket( lockId, pLockBucket );
         PD_RC_CHECK( rc, PDERROR, "Failed to get the lock-bucket, "
                      "test X-lock failed(rc=%d)", rc );
         rc = pLockBucket->test( eduCB, lockId, DPS_TRANSLOCK_X );
      }
      PD_RC_CHECK( rc, PDINFO, "Failed to test the X-lock(rc=%d)", rc );

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_TESTX );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_TESTIX, "dpsTransLock::testIX" )
   INT32 dpsTransLock::testIX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_TESTIX ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      SDB_ASSERT( ( DMS_INVALID_OFFSET == lockId._recordOffset
                  && DMS_INVALID_EXTENT == lockId._recordExtentID ),
                  "IX-Lock only used for collection or collectionspace" ) ;
      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;
      dpsTransLockId sLockId;
      dpsTransCBLockInfo *pLockInfo = NULL;

      pLockInfo = eduCB->getTransLock( lockId );
      if ( pLockInfo != NULL && pLockInfo->isLockMatch( DPS_TRANSLOCK_IX ))
      {
         goto done;
      }

      if ( lockId._collectionID != DMS_INVALID_MBID )
      {
         sLockId._logicCSID = lockId._logicCSID;
         rc = testIS( eduCB, sLockId );
         PD_RC_CHECK( rc, PDINFO, "Failed to test the IS-lock of space, "
                      "test IX-lock failed(rc=%d)", rc );
      }

      if ( pLockInfo )
      {
         rc = testUpgrade( eduCB, lockId, pLockInfo, DPS_TRANSLOCK_IX );
      }
      else
      {
         rc = getBucket( lockId, pLockBucket );
         PD_RC_CHECK( rc, PDERROR, "Failed to get the lock-bucket, "
                      "test IX-lock failed(rc=%d)", rc );

         rc = pLockBucket->test( eduCB, lockId, DPS_TRANSLOCK_IX );
      }
      PD_RC_CHECK( rc, PDINFO, "Failed to test the IX-lock(rc=%d)", rc );

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_TESTIX );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_TRYX, "dpsTransLock::tryX" )
   INT32 dpsTransLock::tryX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY ( SDB_DPSTRANSLOCK_TRYX ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;
      dpsTransLockId iLockId;
      dpsTransCBLockInfo *pLockInfo = NULL;
      BOOLEAN isIXLocked = FALSE;

      if ( lockId._collectionID != DMS_INVALID_MBID )
      {
         iLockId = lockId ;
         if ( lockId._recordOffset != DMS_INVALID_OFFSET )
         {
            iLockId._recordExtentID = DMS_INVALID_EXTENT;
            iLockId._recordOffset = DMS_INVALID_OFFSET;
         }
         else
         {
            iLockId._collectionID = DMS_INVALID_MBID;
         }
         rc = tryIX( eduCB, iLockId);
   
         PD_RC_CHECK( rc, PDERROR, "Failed to get the intention-lock, "
                      "get X-lock failed(rc=%d)", rc );
         isIXLocked = TRUE;
      }

      pLockInfo = eduCB->getTransLock( lockId );
      if ( pLockInfo != NULL && pLockInfo->isLockMatch( DPS_TRANSLOCK_X ))
      {
         pLockInfo->incRef();
         goto done;
      }

      if ( pLockInfo )
      {
         rc = tryUpgrade( eduCB, lockId, pLockInfo, DPS_TRANSLOCK_X );
         if ( SDB_OK == rc )
         {
            pLockInfo->incRef();
         }
      }
      else
      {
         rc = getBucket( lockId, pLockBucket );
         PD_RC_CHECK( rc, PDERROR, "Failed to get the lock-bucket, "
                      "get X-lock failed(rc=%d)", rc );
         eduCB->addLockInfo( lockId, DPS_TRANSLOCK_X );
         rc = pLockBucket->tryAcquire( eduCB, lockId, DPS_TRANSLOCK_X );
         if ( rc )
         {
            eduCB->delLockInfo( lockId );
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get the X-lock(rc=%d)", rc );

      PD_LOG( PDDEBUG, "Get the X-lock(%s)", lockId.toString().c_str() );

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_TRYX );
      return rc;
   error:
      if ( isIXLocked )
      {
         release( eduCB, iLockId );
      }
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_TRYS, "dpsTransLock::tryS" )
   INT32 dpsTransLock::tryS( _pmdEDUCB *eduCB, const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY ( SDB_DPSTRANSLOCK_TRYS ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;
      dpsTransLockId iLockId;
      dpsTransCBLockInfo *pLockInfo = NULL;
      BOOLEAN isISLocked = FALSE;

      if ( lockId._collectionID != DMS_INVALID_MBID )
      {
         iLockId = lockId ;
         if ( lockId._recordOffset != DMS_INVALID_OFFSET )
         {
            iLockId._recordExtentID = DMS_INVALID_EXTENT;
            iLockId._recordOffset = DMS_INVALID_OFFSET;
         }
         else
         {
            iLockId._collectionID = DMS_INVALID_MBID;
         }
         rc = tryIS( eduCB, iLockId);
   
         PD_RC_CHECK( rc, PDERROR, "Failed to get the intention-lock, "
                      "get S-lock failed(rc=%d)", rc );
         isISLocked = TRUE;
      }

      pLockInfo = eduCB->getTransLock( lockId );
      if ( pLockInfo != NULL && pLockInfo->isLockMatch( DPS_TRANSLOCK_S ))
      {
         pLockInfo->incRef();
         goto done;
      }

      if ( pLockInfo )
      {
         rc = tryUpgrade( eduCB, lockId, pLockInfo, DPS_TRANSLOCK_S );
         if ( SDB_OK == rc )
         {
            pLockInfo->incRef();
         }
      }
      else
      {
         rc = getBucket( lockId, pLockBucket );
         PD_RC_CHECK( rc, PDERROR, "Failed to get the lock-bucket, "
                      "get S-lock failed(rc=%d)", rc );
         eduCB->addLockInfo( lockId, DPS_TRANSLOCK_S);
         rc = pLockBucket->tryAcquire( eduCB, lockId, DPS_TRANSLOCK_S );
         if ( rc )
         {
            eduCB->delLockInfo( lockId );
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get the S-lock(rc=%d)", rc );

      PD_LOG( PDDEBUG, "Get the S-lock(%s)", lockId.toString().c_str() );

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_TRYS );
      return rc;
   error:
      if ( isISLocked )
      {
         release( eduCB, iLockId );
      }
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_TRYIX, "dpsTransLock::tryIX" )
   INT32 dpsTransLock::tryIX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_TRYIX ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      SDB_ASSERT( ( DMS_INVALID_OFFSET == lockId._recordOffset
                  && DMS_INVALID_EXTENT == lockId._recordExtentID ),
                  "IX-Lock only used for collection or collectionspace" ) ;
      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;
      dpsTransLockId sLockId;
      dpsTransCBLockInfo *pLockInfo = NULL;
      BOOLEAN isISLocked = FALSE;

      if ( lockId._collectionID != DMS_INVALID_MBID )
      {
         sLockId._logicCSID = lockId._logicCSID;
         rc = tryIS( eduCB, sLockId );
         PD_RC_CHECK( rc, PDERROR, "Failed to get the S-lock of space, "
                      "get IX-lock failed(rc=%d)", rc );
         isISLocked = TRUE;
      }

      pLockInfo = eduCB->getTransLock( lockId );
      if ( pLockInfo != NULL && pLockInfo->isLockMatch( DPS_TRANSLOCK_IX ))
      {
         pLockInfo->incRef();
         goto done;
      }

      if ( pLockInfo )
      {
         rc = tryUpgrade( eduCB, lockId, pLockInfo, DPS_TRANSLOCK_IX );
         if ( SDB_OK == rc )
         {
            pLockInfo->incRef();
         }
      }
      else
      {
         rc = getBucket( lockId, pLockBucket );
         PD_RC_CHECK( rc, PDERROR, "Failed to get the lock-bucket, "
                      "get IX-lock failed(rc=%d)", rc );

         eduCB->addLockInfo( lockId, DPS_TRANSLOCK_IX );
         rc = pLockBucket->tryAcquire( eduCB, lockId, DPS_TRANSLOCK_IX );
         if ( rc )
         {
            eduCB->delLockInfo( lockId );
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get the IX-lock(rc=%d)", rc );

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_TRYIX );
      return rc;
   error:
      if ( isISLocked )
      {
         release( eduCB, sLockId );
      }
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_TRYIS, "dpsTransLock::tryIS" )
   INT32 dpsTransLock::tryIS( _pmdEDUCB *eduCB, const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_TRYIS ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      SDB_ASSERT( ( DMS_INVALID_OFFSET == lockId._recordOffset
                  && DMS_INVALID_EXTENT == lockId._recordExtentID ),
                  "IX-Lock only used for collection or collectionspace" ) ;
      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;
      dpsTransLockId sLockId;
      dpsTransCBLockInfo *pLockInfo = NULL;
      BOOLEAN isISLocked = FALSE;

      if ( lockId._collectionID != DMS_INVALID_MBID )
      {
         sLockId._logicCSID = lockId._logicCSID;
         rc = tryIS( eduCB, sLockId );
         PD_RC_CHECK( rc, PDERROR, "Failed to get the S-lock of space, "
                      "get IS-lock failed(rc=%d)", rc );
         isISLocked = TRUE;
      }

      pLockInfo = eduCB->getTransLock( lockId );
      if ( pLockInfo != NULL )
      {
         pLockInfo->incRef();
         goto done;
      }

      rc = getBucket( lockId, pLockBucket );
      PD_RC_CHECK( rc, PDERROR, "Failed to get the lock-bucket, "
                   "get IS-lock failed(rc=%d)", rc );

      eduCB->addLockInfo( lockId, DPS_TRANSLOCK_IS );
      rc = pLockBucket->tryAcquire( eduCB, lockId, DPS_TRANSLOCK_IS );
      if ( rc )
      {
         eduCB->delLockInfo( lockId );
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get the IS-lock(rc=%d)", rc );

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_TRYIS );
      return rc;
   error:
      if ( isISLocked )
      {
         release( eduCB, sLockId );
      }
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_TRYUPGRADE, "dpsTransLock::tryUpgrade" )
   INT32 dpsTransLock::tryUpgrade( _pmdEDUCB *eduCB,
                                   const dpsTransLockId &lockId,
                                   dpsTransCBLockInfo *pLockInfo,
                                   DPS_TRANSLOCK_TYPE lockType )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_TRYUPGRADE ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      INT32 rc = SDB_OK;
      DPS_TRANSLOCK_TYPE lastLockType;
      dpsLockBucket *pLockBucket = NULL;

      rc = upgradeCheck( pLockInfo->getType(), lockType );
      PD_RC_CHECK( rc, PDERROR, "Upgrade lock failed(rc=%d)", rc );

      rc = getBucket( lockId, pLockBucket );
      PD_RC_CHECK( rc, PDERROR, "Failed to get lock-bucket, upgrade lock "
                   "failed(rc=%d)", rc );

      lastLockType = pLockInfo->getType();
      pLockInfo->setType( lockType );
      rc = pLockBucket->tryAcquire( eduCB, lockId, lockType );
      PD_CHECK( SDB_OK == rc, rc, rollbackType, PDERROR,
                "Upgrade lock failed(rc=%d)", rc );

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_TRYUPGRADE );
      return rc;
   rollbackType:
      pLockInfo->setType( lastLockType );
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_TRYUPGRADEORAPPEND, "dpsTransLock::tryUpgradeOrAppend" )
   INT32 dpsTransLock::tryUpgradeOrAppendHead( _pmdEDUCB *eduCB,
                                               const dpsTransLockId &lockId,
                                               dpsTransCBLockInfo *pLockInfo,
                                               DPS_TRANSLOCK_TYPE lockType )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_TRYUPGRADEORAPPEND ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      INT32 rc = SDB_OK;
      DPS_TRANSLOCK_TYPE lastLockType;
      dpsLockBucket *pLockBucket = NULL;

      rc = upgradeCheck( pLockInfo->getType(), lockType );
      PD_RC_CHECK( rc, PDERROR, "Upgrade lock failed(rc=%d)", rc );

      rc = getBucket( lockId, pLockBucket );
      PD_RC_CHECK( rc, PDERROR, "Failed to get lock-bucket, upgrade lock "
                   "failed(rc=%d)", rc );

      lastLockType = pLockInfo->getType();
      pLockInfo->setType( lockType );
      rc = pLockBucket->tryAcquireOrAppend( eduCB, lockId, lockType, TRUE );
      if ( rc != SDB_OK )
      {
         if ( SDB_DPS_TRANS_APPEND_TO_WAIT == rc )
         {
            goto done ;
         }
         PD_LOG( PDERROR, "Upgrade lock failed(rc=%d)", rc );
         goto rollbackType;
      }
   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_TRYUPGRADEORAPPEND );
      return rc;
   rollbackType:
      pLockInfo->setType( lastLockType );
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_TRYORAPPENDX, "dpsTransLock::tryOrAppendX" )
   INT32 dpsTransLock::tryOrAppendX( _pmdEDUCB *eduCB,
                                     const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_TRYORAPPENDX ) ;
      SDB_ASSERT( eduCB, "eduCB can't be null" ) ;
      SDB_ASSERT( lockId._collectionID != DMS_INVALID_MBID,
                  "invalid collectionID" ) ;
      SDB_ASSERT(( lockId._recordExtentID != DMS_INVALID_EXTENT &&
                   lockId._recordOffset != DMS_INVALID_OFFSET ),
                  "invalid recordID" ) ;

      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;
      dpsTransLockId iLockId;
      dpsTransCBLockInfo *pLockInfo = NULL;

      iLockId = lockId ;
      iLockId._recordExtentID = DMS_INVALID_EXTENT;
      iLockId._recordOffset = DMS_INVALID_OFFSET;
      rc = tryIX( eduCB, iLockId);
      PD_RC_CHECK( rc, PDERROR, "Failed to get the intention-lock, "
                   "get X-lock failed(rc=%d)", rc );

      pLockInfo = eduCB->getTransLock( lockId );
      if ( pLockInfo != NULL && pLockInfo->isLockMatch( DPS_TRANSLOCK_X ))
      {
         pLockInfo->incRef();
         goto done;
      }

      if ( pLockInfo )
      {
         rc = tryUpgradeOrAppendHead( eduCB, lockId, pLockInfo, DPS_TRANSLOCK_X );
         if ( SDB_OK == rc )
         {
            pLockInfo->incRef();
         }
      }
      else
      {
         rc = getBucket( lockId, pLockBucket );
         PD_CHECK( SDB_OK == rc, rc, errorclear, PDERROR,
                   "Failed to get the lock-bucket, get X-lock failed(rc=%d)",
                   rc );
         eduCB->addLockInfo( lockId, DPS_TRANSLOCK_X );
         rc = pLockBucket->tryAcquireOrAppend( eduCB, lockId, DPS_TRANSLOCK_X );
         if ( rc )
         {
            if ( SDB_DPS_TRANS_APPEND_TO_WAIT == rc )
            {
               goto done;
            }
            eduCB->delLockInfo( lockId );
         }
      }
      PD_CHECK( SDB_OK == rc, rc, errorclear, PDERROR,
                "Failed to get the X-lock(rc=%d)", rc );

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_TRYORAPPENDX );
      return rc;
   errorclear:
      release( eduCB, iLockId );
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_WAIT, "dpsTransLock::wait" )
   INT32 dpsTransLock::wait( _pmdEDUCB *eduCB, const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_WAIT ) ;
      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;
      rc = getBucket( lockId, pLockBucket );
      PD_CHECK( SDB_OK == rc, rc, error, PDERROR,
                "Failed to get the lock-bucket, get X-lock failed(rc=%d)",
                rc );
      rc = pLockBucket->waitLockX( eduCB, lockId );
      PD_RC_CHECK( rc, PDERROR, "Wait lock failed(rc=%d)", rc );

   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_WAIT );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSTRANSLOCK_HASWAIT, "dpsTransLock::hasWait" )
   BOOLEAN dpsTransLock::hasWait( const dpsTransLockId &lockId )
   {
      PD_TRACE_ENTRY( SDB_DPSTRANSLOCK_HASWAIT ) ;
      BOOLEAN result = FALSE;
      INT32 rc = SDB_OK;
      dpsLockBucket *pLockBucket = NULL;
      rc = getBucket( lockId, pLockBucket );
      if ( rc || NULL == pLockBucket )
      {
         result = FALSE;
         goto done;
      }
      result = pLockBucket->hasWait( lockId );
   done:
      PD_TRACE_EXIT ( SDB_DPSTRANSLOCK_HASWAIT );
      return result ;
   }

   DPS_TRANS_WAIT_LOCK::DPS_TRANS_WAIT_LOCK( _pmdEDUCB *eduCB, const dpsTransLockId & lockId )
   {
      if ( eduCB != NULL )
      {
         _eduCB = eduCB ;
         _eduCB->setWaitLock( lockId ) ;
      }
   }

   DPS_TRANS_WAIT_LOCK::DPS_TRANS_WAIT_LOCK( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                             UINT16 collectionID,
                                             const _dmsRecordID *recordID )
   {
      if ( eduCB != NULL )
      {
         _eduCB = eduCB ;
         dpsTransLockId lockId( logicCSID, collectionID, recordID ) ;
         _eduCB->setWaitLock( lockId ) ;
      }
   }

   DPS_TRANS_WAIT_LOCK::~DPS_TRANS_WAIT_LOCK()
   {
      if ( _eduCB != NULL )
      {
         _eduCB->clearWaitLock() ;
         _eduCB = NULL ;
      }
   }

}

