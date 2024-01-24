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

   Source File Name = dmsIndexSortingBuilder.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsIndexSortingBuilder.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsStorageIndex.hpp"
#include "dmsStorageData.hpp"
#include "dmsScanner.hpp"
#include "ixmKey.hpp"
#include "ixm.hpp"
#include "dmsCB.hpp"
#include "pmdEDU.hpp"
#include "dmsTrace.hpp"
#include "rtnDiskTBScanner.hpp"

namespace engine
{

   _dmsIndexSortingBuilder::_dmsIndexSortingBuilder( _dmsStorageUnit* su,
                                                     _dmsMBContext* mbContext,
                                                     _pmdEDUCB* eduCB,
                                                     dmsExtentID indexExtentID,
                                                     dmsExtentID indexLogicID,
                                                     INT32 sortBufferSize,
                                                     dmsIndexBuildGuardPtr &guardPtr,
                                                     dmsDupKeyProcessor *dkProcessor,
                                                     dmsIdxTaskStatus* pIdxStatus )
   : _dmsIndexBuilder( su, mbContext, eduCB, indexExtentID, indexLogicID,
                       guardPtr, dkProcessor, pIdxStatus )
   {
      _sorter = NULL ;
      _eoc = FALSE ;
      _bufSize = (INT64)sortBufferSize * 1024 * 1024 ;

      // add extend size for fetching records by extent granularity
      // and to prevent sorter's buffer overflowing.
      // so we assign max extent size to ensure the sorter can't
      // overflow when fetching records from a extent.
      _bufExtSize = DMS_MAX_EXTENT_SZ( (su->data()) ) ;
      _needBuildByRange = FALSE ;
   }

   _dmsIndexSortingBuilder::~_dmsIndexSortingBuilder()
   {
      if ( NULL != _sorter )
      {
         sdbGetDMSCB()->releaseIxmKeySorter( _sorter ) ;
         _sorter = NULL ;
      }
   }

   INT32 _dmsIndexSortingBuilder::_init()
   {
      INT32 rc = SDB_OK ;

      Ordering ordering = Ordering::make( _indexCB->keyPattern() ) ;
      _dmsIxmKeyComparer comparer( ordering ) ;

      rc = sdbGetDMSCB()->createIxmKeySorter( _bufSize + _bufExtSize, comparer,
                                              &_sorter ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG( PDERROR, "Failed to create ixmKey sorter, rc: %d", rc ) ;
         goto error ;
      }

      _needBuildByRange = _indexCB->notArray() ? FALSE : TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsIndexSortingBuilder::_fillSorter( rtnTBScanner &scanner,
                                               dmsRecordID &minRID,
                                               dmsRecordID &maxRID,
                                               dmsRecordID &moveRID,
                                               ossScopedRWLock &lock )
   {
      INT32 rc = SDB_OK ;

      minRID.reset() ;
      maxRID.reset() ;

      for(;;)
      {
         if ( scanner.isEOF() )
         {
            if ( _eoc )
            {
               rc = SDB_DMS_EOC ;
               goto error ;
            }
            else
            {
               _eoc = TRUE ;
               rc = SDB_OK ;
               goto done ;
            }
         }

         if ( _sorter->usedBufferSize() > _bufSize )
         {
            // the sorter's buffer is logical overflow (actually not overflow
            // in sorter), so stop fetching records from next extent
            PD_LOG( PDDEBUG, "sorter is full, "
                    "bufSize=%lld, usedBufSize=%lld, total=%lld", _bufSize,
                    _sorter->usedBufferSize(), _sorter->bufferSize() ) ;
            goto done ;
         }

         rc = _beforeExtent() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check before scanner, rc: %d", rc ) ;

         dmsDataScanner extScanner( _suData, _mbContext, &scanner, NULL ) ;
         _mthRecordGenerator generator ;
         dmsRecordID processedRID ;
         dmsRecordID recordID ;
         ossValuePtr recordDataPtr ;
         UINT64 scannedNum = 0 ;

         while ( SDB_OK == ( rc = extScanner.advance( recordID, generator, _eduCB ) ) )
         {
            BSONObjSet keySet ;
            BSONObjSet::iterator it ;
            generator.getDataPtr( recordDataPtr ) ;

            BOOLEAN needMove = FALSE ;
            rc = _buildGuardPtr->buildCheckMove( recordID, needMove) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check index build, rc: %d", rc ) ;
            if ( needMove )
            {
               moveRID = recordID ;
               break ;
            }

            rc = _getKeySet( recordDataPtr, keySet ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            for ( it = keySet.begin() ; it != keySet.end() ; it++ )
            {
               ixmKeyOwned key( *(it) ) ;
               rc = _sorter->push( key, recordID ) ;
               if ( SDB_OK != rc )
               {
                  SDB_ASSERT( SDB_DMS_EOC != rc, "sorter can't overflow" ) ;
                  goto error ;
               }
            }

            processedRID = recordID ;
            if ( !minRID.isValid() )
            {
               minRID = recordID ;
            }
         }

         if ( processedRID.isValid() )
         {
            maxRID = processedRID ;
         }

         if ( SDB_DMS_EOC != rc && SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get record: %d", rc ) ;
            goto error ;
         }
         if ( ( !moveRID.isValid() ) &&
              ( scanner.isEOF() ) )
         {
            lock.lock( EXCLUSIVE ) ;
            dmsRecordID nextRID ;
            if ( maxRID.isValid() )
            {
               nextRID.fromUINT64( maxRID.toUINT64() + 1 ) ;
            }
            else
            {
               dmsRecordID lastScanRID = _indexCB->getScanRID() ;
               if ( lastScanRID.isValid() )
               {
                  nextRID.fromUINT64( lastScanRID.toUINT64() + 1 ) ;
               }
               else
               {
                  nextRID.resetMin() ;
               }
            }

            rc = scanner.relocateRID( nextRID ) ;
            if ( SDB_OK != rc )
            {
               if ( SDB_DMS_EOC != rc )
               {
                  PD_LOG ( PDERROR, "Failed to relocate record: %d", rc ) ;
                  goto error ;
               }
               rc = SDB_OK ;
            }
         }

         rc = _afterExtent( processedRID, scannedNum, scanner.isEOF() ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         rc = scanner.pauseScan() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to pause scanner, rc: %d", rc ) ;

         if ( moveRID.isValid() )
         {
            break ;
         }
      }

   done:
      if ( minRID.isValid() && !maxRID.isValid() )
      {
         maxRID = minRID ;
      }
      return rc ;
   error:
      goto done ;
   }


   INT32 _dmsIndexSortingBuilder::_insertKeys( const dmsRecordID &minRID,
                                               const dmsRecordID &maxRID,
                                               const Ordering& ordering,
                                               ossScopedRWLock &lock )
   {
      #define _KEYS_PER_BATCH 10000
      INT32 rc = SDB_OK ;

      if ( _needBuildByRange )
      {
         _buildGuardPtr->buildSetRange( minRID, maxRID ) ;
      }

      for (;;)
      {
         ixmKey key ;
         dmsRecordID recordID ;

         if ( _eduCB->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         for ( INT32 i = 0 ; i < _KEYS_PER_BATCH ; i++ )
         {
            rc = _sorter->fetch( key, recordID ) ;
            if ( SDB_OK == rc )
            {
               BOOLEAN needProcess = FALSE, needWait = FALSE, needMove = FALSE ;
               rc = _buildGuardPtr->buildCheck( recordID, _needBuildByRange, needProcess, needWait, needMove) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to check index build, rc: %d", rc ) ;

               if ( needMove )
               {
                  SDB_ASSERT( FALSE, "Should not be here" ) ;
               }
               else if ( needWait )
               {
                  lock.lock( EXCLUSIVE ) ;
                  rc = _buildGuardPtr->buildCheckAfterWait( recordID, needProcess ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to check index build, rc: %d", rc ) ;
               }

               if ( needProcess )
               {
                  _bufBuilder.reset() ;
                  rc = _insertKey( key.toBson( &_bufBuilder ), recordID, ordering ) ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }
               }

               _buildGuardPtr->buildDone( _needBuildByRange, recordID ) ;
            }
            else if ( SDB_DMS_EOC != rc )
            {
               goto error ;
            }
            else
            {
               rc = SDB_OK ;
               goto done ;
            }
         }

         // check if scanner is interrupted
         rc = _checkInterrupt() ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

   done:
      _buildGuardPtr->buildExit() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsIndexSortingBuilder::_build()
   {
      INT32 rc = SDB_OK ;

      dmsRecordID moveRID ;
      Ordering ordering = Ordering::make( _indexCB->keyPattern() ) ;
      rtnDiskTBScanner scanner( _su, _mbContext, _scanRID, TRUE, 1, FALSE, _eduCB ) ;

      rc = _init() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      for(;;)
      {
         dmsRecordID minRID, maxRID ;
         if ( _eoc )
         {
            goto done ;
         }

         rc = _sorter->reset() ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         rc = _checkIndexAfterLock( SHARED ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         if ( moveRID.isValid() )
         {
            {
               ossScopedRWLock lock( &( _buildGuardPtr->getProcessMutex() ), EXCLUSIVE ) ;
               rc = _buildGuardPtr->buildMove( moveRID ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to move index build record ID, rc: %d", rc ) ;
            }
            rc = scanner.relocateRID( moveRID ) ;
            if ( SDB_OK != rc )
            {
               if ( SDB_DMS_EOC != rc )
               {
                  PD_LOG ( PDERROR, "Failed to get record: %d", rc ) ;
                  goto error ;
               }
               rc = SDB_OK ;
               goto done ;
            }
            moveRID.reset() ;
         }

         {
            OSS_LATCH_MODE mode = scanner.isInit() ? SHARED : EXCLUSIVE ;
            ossScopedRWLock lock( &( _buildGuardPtr->getProcessMutex() ), mode ) ;

            if ( _pIdxStatus )
            {
               _pIdxStatus->setOpInfo( OPINFO_SCAN_DATA ) ;
            }

            rc = _fillSorter( scanner, minRID, maxRID, moveRID, lock ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            if ( _pIdxStatus )
            {
               _pIdxStatus->setOpInfo( OPINFO_SORT_DATA ) ;
            }

            rc = _sorter->sort() ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            // check if scanner is interrupted
            rc = _checkInterrupt() ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            if ( _pIdxStatus )
            {
               _pIdxStatus->setOpInfo( OPINFO_INSERT_KEY ) ;
            }

            rc = _insertKeys( minRID, maxRID, ordering, lock ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }

         _mbContext->mbUnlock() ;

         // TODO: sleep lms to let others have more opportunities
         // to get the write lock. THIS is only a temporary solution,
         // we need a fairer lock to solve this problem
         ossSleep(1) ;
      }

   done:
      if ( _pIdxStatus )
      {
         _pIdxStatus->resetOpInfo() ;
      }
      _mbContext->mbUnlock() ;
      return rc ;
   error:
      goto done;
   }

}

