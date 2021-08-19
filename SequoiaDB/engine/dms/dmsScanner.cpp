/*******************************************************************************


   Copyright (C) 2011-2019 SequoiaDB Ltd.

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

   Source File Name = dmsScanner.cpp

   Descriptive Name = Data Management Service Scanner

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/08/2013  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsScanner.hpp"
#include "dmsStorageIndex.hpp"
#include "dmsStorageDataCommon.hpp"
#include "rtnIXScanner.hpp"
#include "rtnDiskIXScanner.hpp"
#include "rtnMergeIXScanner.hpp"
#include "rtnIXScannerFactory.hpp"
#include "bpsPrefetch.hpp"
#include "dmsCompress.hpp"
#include "dmsTransContext.hpp"
#include "dpsTransLockMgr.hpp"
#include "dpsTransExecutor.hpp"
#include "dmsTransLockCallback.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"


using namespace bson ;

namespace engine
{

   #define DMS_IS_WRITE_OPR(accessType)   \
      ( DMS_ACCESS_TYPE_UPDATE == accessType || \
        DMS_ACCESS_TYPE_DELETE == accessType ||\
        DMS_ACCESS_TYPE_INSERT == accessType )

   #define DMS_IS_READ_OPR(accessType) \
      ( DMS_ACCESS_TYPE_QUERY == accessType || \
        DMS_ACCESS_TYPE_FETCH == accessType )

   /*
      _dmsScanner implement
   */
   _dmsScanner::_dmsScanner( dmsStorageDataCommon *su, dmsMBContext *context,
                             mthMatchRuntime *matchRuntime,
                             DMS_ACCESS_TYPE accessType )
   {
      SDB_ASSERT( su, "storage data can't be NULL" ) ;
      SDB_ASSERT( context, "context can't be NULL" ) ;
      _pSu = su ;
      _context = context ;
      _matchRuntime = matchRuntime ;
      _accessType = accessType ;
      _mbLockType = SHARED ;
      _transIsolation = TRANS_ISOLATION_RU ;
      _waitLock = FALSE ;
      _useRollbackSegment = TRUE ;

      if ( DMS_IS_WRITE_OPR( _accessType ) )
      {
         _mbLockType = EXCLUSIVE ;
      }
   }

   _dmsScanner::~_dmsScanner()
   {
      _context    = NULL ;
      _pSu        = NULL ;
   }

   /*
      _dmsExtScannerBase implement
   */
   _dmsExtScannerBase::_dmsExtScannerBase( dmsStorageDataCommon *su,
                                           dmsMBContext *context,
                                           mthMatchRuntime *matchRuntime,
                                           dmsExtentID curExtentID,
                                           DMS_ACCESS_TYPE accessType,
                                           INT64 maxRecords,
                                           INT64 skipNum,
                                           INT32 flag )
   :_dmsScanner( su, context, matchRuntime, accessType ),
    _curRecordPtr( NULL )
   {
      _maxRecords          = maxRecords ;
      _skipNum             = skipNum ;
      _next                = DMS_INVALID_OFFSET ;
      _firstRun            = TRUE ;
      _hasLockedRecord     = FALSE ;
      _extent              = NULL ;
      _pTransCB            = NULL ;
      _curRID._extent      = curExtentID ;
      _recordLock          = DPS_TRANSLOCK_MAX ;
      _needUnLock          = FALSE ;
      _selectForUpdate     = FALSE ;
      _CSCLLockHeld        = FALSE ;
      _cb                  = NULL ;

      if ( OSS_BIT_TEST( flag, FLG_QUERY_FOR_UPDATE ) )
      {
         _selectForUpdate = TRUE ;
      }
   }

   _dmsExtScannerBase::~_dmsExtScannerBase ()
   {
      _extent     = NULL ;

      if ( FALSE == _firstRun && _recordLock != DPS_TRANSLOCK_MAX &&
           _hasLockedRecord &&
           DMS_INVALID_OFFSET != _curRID._offset )
      {
         _pTransCB->transLockRelease( _cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID, &_callback ) ;
         _hasLockedRecord = FALSE ;
      }

      releaseCSCLLock() ;
   }

   dmsTransLockCallback* _dmsExtScannerBase::callbackHandler()
   {
      return &_callback ;
   }

   const dmsTransRecordInfo* _dmsExtScannerBase::recordInfo() const
   {
      return _callback.getTransRecordInfo() ;
   }

   dmsExtentID _dmsExtScannerBase::nextExtentID() const
   {
      if ( _extent )
      {
         return _extent->_nextExtent ;
      }
      return DMS_INVALID_EXTENT ;
   }

   INT32 _dmsExtScannerBase::stepToNextExtent()
   {
      if ( 0 != _maxRecords &&
           DMS_INVALID_EXTENT != nextExtentID() )
      {
         _curRID._extent = nextExtentID() ;
         releaseCSCLLock() ;
         _firstRun = TRUE ;
         return SDB_OK ;
      }
      return SDB_DMS_EOC ;
   }

   void _dmsExtScannerBase::_checkMaxRecordsNum( _mthRecordGenerator &generator )
   {
      if ( _maxRecords > 0 )
      {
         if ( _maxRecords >= generator.getRecordNum() )
         {
            _maxRecords -= generator.getRecordNum() ;
         }
         else
         {
            INT32 num = generator.getRecordNum() - _maxRecords ;
            generator.popTail( num ) ;
            _maxRecords = 0 ;
         }
      }
   }

   INT32 _dmsExtScannerBase::advance( dmsRecordID &recordID,
                                      _mthRecordGenerator &generator,
                                      pmdEDUCB *cb,
                                      _mthMatchTreeContext *mthContext )
   {
      INT32 rc = SDB_OK ;

      if ( _firstRun )
      {
         rc = _firstInit( cb ) ;
         PD_RC_CHECK( rc, PDWARNING, "first init failed, rc: %d", rc ) ;
      }
      // have locked, but not trans, need to release record lock held
      // from last round of scan
      else if ( _needUnLock && _hasLockedRecord &&
                DMS_INVALID_OFFSET != _curRID._offset )
      {
         _pTransCB->transLockRelease( cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID, &_callback ) ;
         _hasLockedRecord = FALSE ;
      }

      rc = _fetchNext( recordID, generator, cb, mthContext ) ;
      if ( rc )
      {
         // Do not write error log when EOC.
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Get next record failed, rc: %d", rc ) ;
         }
         goto error ;
      }

   done:
      return rc ;
   error:
      recordID.reset() ;
      _curRID._offset = DMS_INVALID_OFFSET ;
      goto done ;
   }

   void _dmsExtScannerBase::stop()
   {
      if ( FALSE == _firstRun && _recordLock != DPS_TRANSLOCK_MAX
           && _hasLockedRecord &&
           DMS_INVALID_OFFSET != _curRID._offset )
      {
         _pTransCB->transLockRelease( _cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID, &_callback ) ;
         _hasLockedRecord = FALSE ;
      }
      // release CSCL lock if held
      releaseCSCLLock() ;

      _next = DMS_INVALID_OFFSET ;
      _curRID._offset = DMS_INVALID_OFFSET ;
   }

   INT32 _dmsExtScannerBase::acquireCSCLLock( )
   {
      INT32 rc = SDB_OK ;
      if ( !_CSCLLockHeld && DPS_TRANSLOCK_MAX != _recordLock )
      {
         dmsTBTransContext tbTxContext( _context, _accessType ) ;
         dpsTransRetInfo   lockConflict ;

         if ( DPS_TRANSLOCK_IS == dpsIntentLockMode( _recordLock ) )
         {
            rc = _pTransCB->transLockGetIS( _cb, _pSu->logicalID(),
                                            _context->mbID(),
                                            & tbTxContext, &lockConflict ) ;
         }
         else if ( DPS_TRANSLOCK_IX == dpsIntentLockMode( _recordLock ) )
         {
            rc = _pTransCB->transLockGetIX( _cb, _pSu->logicalID(),
                                            _context->mbID(),
                                             & tbTxContext, &lockConflict ) ;
         }
         else
         {
            goto done ;
         }

         // this is performance improvement, failed to get lock should not
         // fail the operation
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDWARNING,
                     "Failed to get CS/CL lock, rc: %d"OSS_NEWLINE
                     "Conflict ( representative ):"OSS_NEWLINE
                     "   EDUID:  %llu"OSS_NEWLINE
                     "   TID:    %u"OSS_NEWLINE
                     "   LockId: %s"OSS_NEWLINE
                     "   Mode:   %s"OSS_NEWLINE,
                     rc,
                     lockConflict._eduID,
                     lockConflict._tid,
                     lockConflict._lockID.toString().c_str(),
                     lockModeToString( lockConflict._lockType ) ) ;
            goto error ;
         }
         else
         {
            _CSCLLockHeld = TRUE ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void   _dmsExtScannerBase::releaseCSCLLock( )
   {
      if ( _CSCLLockHeld )
      {
         _pTransCB->transLockRelease( _cb, _pSu->logicalID(),
                                      _context->mbID() );
         _CSCLLockHeld = FALSE ;
      }
   }

   _dmsExtScanner::_dmsExtScanner( dmsStorageDataCommon *su,
                                   _dmsMBContext *context,
                                   mthMatchRuntime *matchRuntime,
                                   dmsExtentID curExtentID,
                                   DMS_ACCESS_TYPE accessType,
                                   INT64 maxRecords,
                                   INT64 skipNum,
                                   INT32 flag )
   : _dmsExtScannerBase( su, context, matchRuntime, curExtentID, accessType,
                         maxRecords, skipNum, flag )
   {
   }

   _dmsExtScanner::~_dmsExtScanner()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEXTSCAN__FIRSTINIT, "_dmsExtScanner::_firstInit" )
   INT32 _dmsExtScanner::_firstInit( pmdEDUCB *cb )
   {
      INT32 rc          = SDB_OK ;
      _pTransCB         = pmdGetKRCB()->getTransCB() ;
      SDB_BPSCB *pBPSCB = pmdGetKRCB()->getBPSCB () ;
      BOOLEAN   bPreLoadEnabled = pBPSCB->isPreLoadEnabled() ;
      dpsTransExecutor *pExe = cb->getTransExecutor() ;

      PD_TRACE_ENTRY ( SDB__DMSEXTSCAN__FIRSTINIT );
      _transIsolation = pExe->getTransIsolation() ;
      _waitLock = pExe->isTransWaitLock() ;
      _useRollbackSegment = pExe->useRollbackSegment() ;

      /// When not support trans
      if ( !_pSu->isTransSupport() )
      {
         _recordLock = DPS_TRANSLOCK_MAX ;
         _selectForUpdate = FALSE ;
      }
      /// When not in transaction
      else if ( DPS_INVALID_TRANS_ID == cb->getTransID() )
      {
         /// When not use trans lock
         if ( !pExe->useTransLock() )
         {
            _recordLock = DPS_TRANSLOCK_MAX ;
            _selectForUpdate = FALSE ;
         }
         /// Write operation should release lock right now
         else if ( DMS_IS_WRITE_OPR( _accessType ) )
         {
            _recordLock = DPS_TRANSLOCK_X ;
            _needUnLock = TRUE ;
            _useRollbackSegment = FALSE ;
         }
         /// Read is always no lock
         else
         {
            _recordLock = DPS_TRANSLOCK_MAX ;
            _selectForUpdate = FALSE ;
         }
      }
      /// In transaction
      else
      {
         if ( cb->isInTransRollback() )
         {
            _recordLock = DPS_TRANSLOCK_MAX ;
            _selectForUpdate = FALSE ;
         }
         else if ( DMS_IS_WRITE_OPR( _accessType ) )
         {
            _recordLock = DPS_TRANSLOCK_X ;
            _needUnLock = FALSE ;
         }
         else if ( TRANS_ISOLATION_RU == _transIsolation &&
                   !_selectForUpdate )
         {
            _recordLock = DPS_TRANSLOCK_MAX ;
            _selectForUpdate = FALSE ;
         }
         else
         {
            _recordLock = _selectForUpdate ? DPS_TRANSLOCK_U :
                                             DPS_TRANSLOCK_S ;
            if ( TRANS_ISOLATION_RS == _transIsolation ||
                 _selectForUpdate )
            {
               _needUnLock = FALSE ;
               _waitLock = TRUE ;
            }
            else
            {
               _needUnLock = TRUE ;
            }
         }
      }

      _extRW = _pSu->extent2RW( _curRID._extent, _context->mbID() ) ;
      _extRW.setNothrow( TRUE ) ;
      _extent = _extRW.readPtr<dmsExtent>() ;
      if ( NULL == _extent )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( cb->isInterrupted() )
      {
         rc = SDB_APP_INTERRUPT ;
         goto error ;
      }
      if ( !_context->isMBLock( _mbLockType ) )
      {
         rc = _context->mbLock( _mbLockType ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb lock failed, rc: %d", rc ) ;
      }
      if ( !dmsAccessAndFlagCompatiblity ( _context->mb()->_flag,
                                           _accessType ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  _context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }
      if ( !_extent->validate( _context->mbID() ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      // set callback info
      _callback.setBaseInfo( _pTransCB, cb ) ;
      _callback.setIDInfo( _pSu->CSID(), _context->mbID(),
                           _pSu->logicalID(),
                           _context->clLID() ) ;

      // send pre-load request
      if ( bPreLoadEnabled && DMS_INVALID_EXTENT != _extent->_nextExtent )
      {
         pBPSCB->sendPreLoadRequest ( bpsPreLoadReq( _pSu->CSID(),
                                                     _pSu->logicalID(),
                                                     _extent->_nextExtent )) ;
      }

      _cb   = cb ;
      _next = _extent->_firstRecordOffset ;

      // As a performance improvement, we are going to acquire the CS and
      // CL lock right in the beginning to avoid extra performance overhead
      // to acquire these locks when acquiring record lock in each step
      // We release and require the lock during pauseScan/resumeScan
      rc = acquireCSCLLock() ;
      if ( rc )
      {
         goto error ;
      }

      // unset first run
      _firstRun = FALSE ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSEXTSCAN__FIRSTINIT, rc );
      return rc ;
   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEXTSCAN__FETCHNEXT, "_dmsExtScanner::_fetchNext" )
   INT32 _dmsExtScanner::_fetchNext( dmsRecordID &recordID,
                                     _mthRecordGenerator &generator,
                                     pmdEDUCB *cb,
                                     _mthMatchTreeContext *mthContext )
   {
      INT32 rc                = SDB_OK ;
      BOOLEAN result          = TRUE ;
      ossValuePtr recordDataPtr ;
      dmsRecordData recordData ;
      BOOLEAN ignoredLock     = FALSE ;

      _hasLockedRecord        = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSEXTSCAN__FETCHNEXT );
      PD_TRACE5 ( SDB__DMSEXTSCAN__FETCHNEXT,
                 PD_PACK_UINT(_needUnLock),
                 PD_PACK_UINT(_mbLockType),
                 PD_PACK_UINT(_accessType),
                 PD_PACK_UINT(_waitLock),
                 PD_PACK_UINT(_recordLock) );

      // skip extent all
      if ( !_matchRuntime && _skipNum > 0 && _skipNum >= _extent->_recCount )
      {
         _skipNum -= _extent->_recCount ;
         _next = DMS_INVALID_OFFSET ;
      }

      while ( DMS_INVALID_OFFSET != _next && 0 != _maxRecords )
      {
         _curRID._offset = _next ;
         _recordRW = _pSu->record2RW( _curRID, _context->mbID() ) ;
         _curRecordPtr = _recordRW.readPtr( 0 ) ;
         _next = _curRecordPtr->getNextOffset() ;
         ignoredLock = FALSE ;

         if ( _recordLock != DPS_TRANSLOCK_MAX )
         {
            dmsTBTransContext tbTxContext( _context, _accessType ) ;
            dpsTransRetInfo   lockConflict ;

            // attach the recordRW in callback
            _callback.attachRecordRW( &_recordRW ) ;
            _callback.clearStatus() ;

            if ( DPS_TRANSLOCK_X == _recordLock )
            {
               rc = _pTransCB->transLockGetX( cb, _pSu->logicalID(),
                                              _context->mbID(), &_curRID,
                                              & tbTxContext, &lockConflict,
                                              &_callback ) ;
            }
            else if ( DPS_TRANSLOCK_U == _recordLock )
            {
               rc = _pTransCB->transLockGetU( cb, _pSu->logicalID(),
                                              _context->mbID(), &_curRID,
                                              &tbTxContext,
                                              &lockConflict ) ;
            }
            /// DPS_TRANSLOCK_S
            else
            {
               if ( !needWaitForLock() )
               {
                  // for new RC logic, we should first test on S lock instead
                  // of directly wait on the record lock. Under the cover,
                  // the lock call back function would try to use the old copy
                  // (previous committed version) if exist
                  rc = _pTransCB->transLockTestSPreempt( cb, _pSu->logicalID(),
                                                         _context->mbID(),
                                                         &_curRID,
                                                         &lockConflict,
                                                         &_callback ) ;
                  ignoredLock = TRUE ;
                  if ( _callback.isSkipRecord() )
                  {
                     // For newly created records by another transaction,
                     // we could still find it through diskIXScan, we will
                     // skip those records without waiting for lock.
                     rc = SDB_OK ;
                     continue ;
                  }
                  if ( _callback.isUseOldVersion() )
                  {
                     rc = SDB_OK ;
                  }
               }

               /// wait lock
               if ( needWaitForLock() || rc )
               {
                  rc = _pTransCB->transLockGetS( cb, _pSu->logicalID(),
                                                 _context->mbID(), &_curRID,
                                                 & tbTxContext,
                                                 &lockConflict ) ;
                  if ( SDB_OK == rc )
                  {
                     ignoredLock = FALSE ;
                  }
               }
            }

            if ( rc )
            {
               PD_LOG( PDERROR,
                       "Failed to get record lock, rc: %d"OSS_NEWLINE
                       "Request Mode:   %s"OSS_NEWLINE
                       "Conflict ( representative ):"OSS_NEWLINE
                       "   EDUID:  %llu"OSS_NEWLINE
                       "   TID:    %u"OSS_NEWLINE
                       "   LockId: %s"OSS_NEWLINE
                       "   Mode:   %s"OSS_NEWLINE,
                       rc,
                       lockModeToString( _recordLock ),
                       lockConflict._eduID,
                       lockConflict._tid,
                       lockConflict._lockID.toString().c_str(),
                       lockModeToString( lockConflict._lockType ) ) ;
               cb->printInfo( EDU_INFO_ERROR, "Failed to get record lock" ) ;
               goto error ;
            }

            if ( !ignoredLock )
            {
               _hasLockedRecord = TRUE ;
            }

            if ( _callback.hasError() )
            {
               rc = _callback.getResult() ;
               PD_LOG( PDERROR, "Occur error in callback, rc: %d", rc ) ;
               goto error ;
            }

            // while we wait on the record lock, we could give up the mblatch,
            // another guy can come in and delete the next record (note that
            // the current record we are working on should be fine). So we
            // need to refresh the next record(_next) based on the record
            // on disk. We might or might not have record lock, but we have
            // mbLatch, so it's a valid version..
            _next = _curRecordPtr->getNextOffset() ;

            // Since we might got an old version of the record from a buffer,
            // let's the _curRecordPtr. Otherwise we might mistakenly skip
            // the record if it's being deleted.
            _curRecordPtr = _recordRW.readPtr( 0 ) ;
         }

         // if this delete is from the same transaction as marking it
         // deleting, we would end up do the delete, but how to tell?
         // If the X lock was newly acquired/granted, we consider it as a
         // new transaction, because the original one has committed and
         // original lock was released. Pass down newXAcquire.
         if ( _curRecordPtr->isDeleting() )
         {
            if ( _recordLock == DPS_TRANSLOCK_X )
            {
               INT32 rc1 = _pSu->deleteRecord( _context, _curRID,
                                               0, cb, NULL, NULL,
                                               _callback.getTransRecordInfo() ) ;
               if ( rc1 )
               {
                  PD_LOG( PDWARNING, "Failed to delete the deleting record, "
                          "rc: %d", rc ) ;
               }
            }

            if ( _hasLockedRecord )
            {
               _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                            _context->mbID(), &_curRID,
                                            &_callback ) ;
               _hasLockedRecord = FALSE ;
            }
            continue ;
         }
         SDB_ASSERT( !_curRecordPtr->isDeleted(), "record can't be deleted" ) ;

         if ( !_matchRuntime && _skipNum > 0 )
         {
            --_skipNum ;
         }
         else
         {

            recordID = _curRID ;
            rc = _pSu->extractData( _context, _recordRW, cb, recordData ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Extract record data failed, rc: %d", rc ) ;
               goto error ;
            }
            recordDataPtr = ( ossValuePtr )recordData.data() ;
            generator.setDataPtr( recordDataPtr ) ;

            // math
            if ( _matchRuntime && _matchRuntime->getMatchTree() )
            {
               result = TRUE ;
               try
               {
                  _mthMatchTree *matcher = _matchRuntime->getMatchTree() ;
                  rtnParamList *parameters = _matchRuntime->getParametersPointer() ;
                  BSONObj obj( recordData.data() ) ;
                  //do not clear dollarlist flag
                  mthContextClearRecordInfoSafe( mthContext ) ;
                  rc = matcher->matches( obj, result, mthContext, parameters ) ;
                  if ( rc )
                  {
                     PD_LOG( PDERROR, "Failed to match record, rc: %d", rc ) ;
                     goto error ;
                  }
                  if ( result )
                  {
                     rc = generator.resetValue( obj, mthContext ) ;
                     PD_RC_CHECK( rc, PDERROR, "resetValue failed:rc=%d", rc ) ;

                     if ( _skipNum > 0 )
                     {
                        if ( _skipNum >= generator.getRecordNum() )
                        {
                           _skipNum -= generator.getRecordNum() ;
                        }
                        else
                        {
                           generator.popFront( _skipNum ) ;
                           _skipNum = 0 ;
                           _checkMaxRecordsNum( generator ) ;

                           goto done ;
                        }
                     }
                     else
                     {
                        _checkMaxRecordsNum( generator ) ;
                        goto done ; // find ok
                     }
                  }
               }
               catch( std::exception &e )
               {
                  PD_LOG ( PDERROR, "Failed to create BSON object: %s",
                           e.what() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
            } // if ( _match )
            else
            {
               try
               {
                  BSONObj obj( recordData.data() ) ;
                  rc = generator.resetValue( obj, mthContext ) ;
                  PD_RC_CHECK( rc, PDERROR, "resetValue failed:rc=%d", rc ) ;
               }
               catch( std::exception &e )
               {
                  rc = SDB_SYS ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to create BSON object: %s",
                               e.what() ) ;
                  goto error ;
               }

               if ( _skipNum > 0 )
               {
                  --_skipNum ;
               }
               else
               {
                  if ( _maxRecords > 0 )
                  {
                     --_maxRecords ;
                  }
                  goto done ; // find ok
               }
            }
         }

         if ( _hasLockedRecord )
         {
            _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                         _context->mbID(), &_curRID,
                                         &_callback ) ;
            _hasLockedRecord = FALSE ;
         }
      } // while

      rc = SDB_DMS_EOC ;
      goto error ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSEXTSCAN__FETCHNEXT, rc );
      return rc ;
   error:
      if ( _hasLockedRecord )
      {
         _pTransCB->transLockRelease( cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID, &_callback ) ;
         _hasLockedRecord = FALSE ;
      }
      releaseCSCLLock() ;

      recordID.reset() ;
      recordDataPtr = 0 ;
      generator.setDataPtr( recordDataPtr ) ;
      _curRID._offset = DMS_INVALID_OFFSET ;
      goto done ;
   }

   _dmsCappedExtScanner::_dmsCappedExtScanner( dmsStorageDataCommon *su,
                                               dmsMBContext *context,
                                               mthMatchRuntime *matchRuntime,
                                               dmsExtentID curExtentID,
                                               DMS_ACCESS_TYPE accessType,
                                               INT64 maxRecords,
                                               INT64 skipNum,
                                               INT32 flag )
   : _dmsExtScannerBase( su, context, matchRuntime, curExtentID, accessType,
                         maxRecords, skipNum, flag )
   {
      _maxRecords = maxRecords ;
      _skipNum = skipNum ;
      _extent = NULL ;
      _curRID._extent = curExtentID ;
      _next = DMS_INVALID_OFFSET ;
      _lastOffset = DMS_INVALID_OFFSET ;
      _firstRun = TRUE ;
      _cb = NULL ;
      _workExtInfo = NULL ;
      _rangeInit = FALSE ;
      _fastScanByID = FALSE ;

      /// not support transaction
      _recordLock = DPS_TRANSLOCK_MAX ;
   }

   _dmsCappedExtScanner::~_dmsCappedExtScanner()
   {
   }

   INT32 _dmsCappedExtScanner::_firstInit( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN inRange = FALSE ;
      _pTransCB = pmdGetKRCB()->getTransCB() ;

      _extRW = _pSu->extent2RW( _curRID._extent, _context->mbID() ) ;
      _extRW.setNothrow( TRUE ) ;
      _extent = _extRW.readPtr<dmsExtent>() ;
      if ( NULL == _extent )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _validateRange( inRange ) ;
      PD_RC_CHECK( rc , PDERROR, "Failed to validate extant range, rc: %d",
                   rc ) ;

      if ( !inRange )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      if ( !_context->isMBLock( _mbLockType ) )
      {
         rc = _context->mbLock( _mbLockType ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb lock failed, rc: %d", rc ) ;
      }

      _workExtInfo =
         (( _dmsStorageDataCapped * )_pSu)->getWorkExtInfo( _context->mbID() ) ;

      // Check if we are about to scan the working extent. As the extent is very
      // big, in order to improve performance, try to avoid getting the last
      // offset again and again from the extent head when in advance.
      if ( _curRID._extent == _workExtInfo->getID() )
      {
         _next = _workExtInfo->_firstRecordOffset ;
         _lastOffset = _workExtInfo->_lastRecordOffset ;
      }
      else
      {
         _next = _extent->_firstRecordOffset ;
         _lastOffset = _extent->_lastRecordOffset ;
      }

      if ( !_extent->validate( _context->mbID() ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      _cb = cb ;
      _firstRun = FALSE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsCappedExtScanner::_fetchNext( dmsRecordID &recordID,
                                           _mthRecordGenerator &generator,
                                           pmdEDUCB *cb,
                                           _mthMatchTreeContext *mthContext )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN result = TRUE ;
      ossValuePtr recordDataPtr ;
      dmsRecordData recordData ;
      const dmsCappedRecord *record = NULL ;
      UINT32 currExtRecNum = 0 ;

      currExtRecNum = ( _curRID._extent == _workExtInfo->getID() ) ?
                       _workExtInfo->_recCount : _extent->_recCount ;

      if ( !_matchRuntime && _skipNum > 0 && _skipNum >= currExtRecNum &&
           ( _curRID._extent != _context->mb()->_lastExtentID ) )
      {
         _skipNum -= currExtRecNum ;
         _next = DMS_INVALID_OFFSET ;
      }

      while ( _next <= _lastOffset && DMS_INVALID_OFFSET != _next &&
              0 != _maxRecords )
      {
         _curRID._offset = _next ;
         _recordRW = _pSu->record2RW( _curRID, _context->mbID() ) ;
         _recordRW.setNothrow( TRUE ) ;
         record= _recordRW.readPtr<dmsCappedRecord>( 0 ) ;
         if ( !record )
         {
            PD_LOG( PDERROR, "Get record failed" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         _next += record->getSize() ;

         if ( !_matchRuntime && _skipNum > 0 )
         {
            --_skipNum ;
         }
         else
         {
            recordID = _curRID ;
            rc = _pSu->extractData( _context, _recordRW, cb, recordData ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Extract record data failed, rc: %d", rc ) ;
               goto error ;
            }
            recordDataPtr = ( ossValuePtr )recordData.data() ;
            generator.setDataPtr( recordDataPtr ) ;

            if ( _matchRuntime && _matchRuntime->getMatchTree() )
            {
               result = TRUE ;
               try
               {
                  _mthMatchTree *matcher = _matchRuntime->getMatchTree() ;
                  rtnParamList *parameters = _matchRuntime->getParametersPointer() ;
                  BSONObj obj( recordData.data() ) ;
                  //do not clear dollarlist flag
                  mthContextClearRecordInfoSafe( mthContext ) ;
                  rc = matcher->matches( obj, result, mthContext, parameters ) ;
                  if ( rc )
                  {
                     PD_LOG( PDERROR, "Failed to match record, rc: %d", rc ) ;
                     goto error ;
                  }
                  if ( result )
                  {
                     rc = generator.resetValue( obj, mthContext ) ;
                     PD_RC_CHECK( rc, PDERROR, "resetValue failed:rc=%d", rc ) ;

                     if ( _skipNum > 0 )
                     {
                        if ( _skipNum >= generator.getRecordNum() )
                        {
                           _skipNum -= generator.getRecordNum() ;
                        }
                        else
                        {
                           generator.popFront( _skipNum ) ;
                           _skipNum = 0 ;
                           _checkMaxRecordsNum( generator ) ;

                           goto done ;
                        }
                     }
                     else
                     {
                        _checkMaxRecordsNum( generator ) ;
                        goto done ; // find ok
                     }
                  }
               }
               catch( std::exception &e )
               {
                  PD_LOG ( PDERROR, "Failed to create BSON object: %s",
                           e.what() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
            } // if ( _match )
            else
            {
               try
               {
                  BSONObj obj( recordData.data() ) ;
                  rc = generator.resetValue( obj, mthContext ) ;
                  PD_RC_CHECK( rc, PDERROR, "resetValue failed, rc: %d", rc ) ;
               }
               catch( std::exception &e )
               {
                  rc = SDB_SYS ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to create BSON object: %s",
                               e.what() ) ;
                  goto error ;
               }

               if ( _maxRecords > 0 )
               {
                  --_maxRecords ;
               }

               goto done ;
            }
         }
      }

      rc =  SDB_DMS_EOC ;
      goto error ;

   done:
      return rc ;
   error:
      goto done ;
   }

   OSS_INLINE dmsExtentID _dmsCappedExtScanner::_idToExtLID( INT64 id )
   {
      SDB_ASSERT( id >= 0, "id can't be negative" ) ;
      return ( id / DMS_CAP_EXTENT_BODY_SZ ) ;
   }

   // Filter out the extents that we really need to scan(in the range of the
   // condition).
   INT32 _dmsCappedExtScanner::_initFastScanRange()
   {
      INT32 rc = SDB_OK ;
      rtnPredicateSet predicateSet ;
      _mthMatchTree *matcher = _matchRuntime->getMatchTree() ;
      rtnParamList *parameters = _matchRuntime->getParametersPointer() ;

      rc = matcher->calcPredicate( predicateSet, parameters ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to calculate predicate, rc: %d",
                   rc ) ;

      {
         rtnPredicate predicate = predicateSet.predicate( DMS_ID_KEY_NAME ) ;
         if ( predicate.isGeneric() )
         {
            _fastScanByID = FALSE ;
         }
         else
         {
            _fastScanByID = TRUE ;
            // Add valid logical id range.
            BSONObj boStartKey = BSON( DMS_ID_KEY_NAME << 0 ) ;
            BSONObj boStopKey = BSON( DMS_ID_KEY_NAME << OSS_SINT64_MAX ) ;

            rc = predicateSet.addPredicate( DMS_ID_KEY_NAME,
                                            boStartKey.firstElement(),
                                            BSONObj::GTE, FALSE, FALSE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add predicate, rc: %d", rc ) ;
            rc = predicateSet.addPredicate( DMS_ID_KEY_NAME,
                                            boStopKey.firstElement(),
                                            BSONObj::LTE, FALSE, FALSE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add predicate, rc: %d", rc ) ;

            predicate = predicateSet.predicate( DMS_ID_KEY_NAME ) ;

            for ( RTN_SSKEY_LIST::iterator itr = predicate._startStopKeys.begin();
                  itr != predicate._startStopKeys.end(); ++itr )
            {
               dmsExtentID startExtLID = DMS_INVALID_EXTENT ;
               dmsExtentID endExtLID = DMS_INVALID_EXTENT ;
               const rtnKeyBoundary &startKey = itr->_startKey ;
               const rtnKeyBoundary &stopKey = itr->_stopKey ;

               startExtLID = _idToExtLID( startKey._bound.numberLong() ) ;
               endExtLID = _idToExtLID( stopKey._bound.numberLong() ) ;

               _rangeSet.insert( std::pair<dmsExtentID, dmsExtentID>( startExtLID,
                                                                      endExtLID ) ) ;
            }
         }
      }
      _rangeInit = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // As there is no index on capped table, query with condition will be slow as
   // we always need to do full table scan. But if _id is specified in the
   // condition, we can first filter out the extents which are in the range of
   // the condition. This will lead to much better performance.
   INT32 _dmsCappedExtScanner::_validateRange( BOOLEAN &inRange )
   {
      INT32 rc = SDB_OK ;

      // If there is no condition, or no _id in the condition, the validation
      // result should always be TRUE.
      inRange = TRUE ;
      if ( _matchRuntime )
      {
         if ( !_rangeInit )
         {
            rc = _initFastScanRange() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to initial fast scan range, "
                         "rc: %d", rc ) ;
         }

         if ( _fastScanByID )
         {
            // Traverse the range set to check if the current extent is in any
            // valid range.
            for ( EXT_RANGE_SET_ITR itr = _rangeSet.begin();
                  itr != _rangeSet.end(); ++itr )
            {
               if ( _extent->_logicID >= itr->first &&
                    _extent->_logicID <= itr->second )
               {
                  inRange = TRUE ;
                  break ;
               }
            }
         }
         else
         {
            inRange = TRUE ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   dmsExtentID _dmsCappedExtScanner::nextExtentID () const
   {
      if ( _extent )
      {
         return _extent->_nextExtent ;
      }
      return DMS_INVALID_EXTENT ;
   }

   /*
      _dmsTBScanner implement
   */
   _dmsTBScanner::_dmsTBScanner( dmsStorageDataCommon *su,
                                 dmsMBContext *context,
                                 mthMatchRuntime *matchRuntime,
                                 DMS_ACCESS_TYPE accessType,
                                 INT64 maxRecords,
                                 INT64 skipNum,
                                 INT32 flag )
   :_dmsScanner( su, context, matchRuntime, accessType )
   {
      _extScanner    = NULL ;
      _curExtentID   = DMS_INVALID_EXTENT ;
      _firstRun      = TRUE ;
      _maxRecords    = maxRecords ;
      _skipNum       = skipNum ;
      _flag          = flag ;
   }

   _dmsTBScanner::~_dmsTBScanner()
   {
      _curExtentID   = DMS_INVALID_EXTENT ;
      if ( _extScanner )
      {
         SDB_OSS_DEL _extScanner ;
      }
   }

   dmsTransLockCallback* _dmsTBScanner::callbackHandler()
   {
      if ( _extScanner )
      {
         return _extScanner->callbackHandler() ;
      }
      return NULL ;
   }

   const dmsTransRecordInfo* _dmsTBScanner::recordInfo() const
   {
      if ( _extScanner )
      {
         return _extScanner->recordInfo() ;
      }
      return NULL ;
   }

   INT32 _dmsTBScanner::_firstInit()
   {
      INT32 rc = SDB_OK ;

      rc = _getExtScanner() ;
      PD_RC_CHECK( rc, PDERROR, "Get extent scanner failed, rc: %d", rc ) ;

      if ( !_context->isMBLock( _extScanner->_mbLockType ) )
      {
         rc = _context->mbLock( _extScanner->_mbLockType ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb lock failed, rc: %d", rc ) ;
      }

      _context->mbStat()->_crudCB.increaseTbScan( 1 ) ;

      _curExtentID = _context->mb()->_firstExtentID ;
      _resetExtScanner() ;
      _firstRun = FALSE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsTBScanner::_resetExtScanner()
   {
      _extScanner->_firstRun = TRUE ;
      _extScanner->_curRID._extent = _curExtentID ;
      _extScanner->releaseCSCLLock() ;
   }

   INT32 _dmsTBScanner::_getExtScanner()
   {
      INT32 rc = SDB_OK ;
      _extScanner = dmsGetScannerFactory()->create( _pSu, _context,
                                                    _matchRuntime,
                                                    _curExtentID,
                                                    _accessType,
                                                    _maxRecords,
                                                    _skipNum,
                                                    _flag ) ;
      if ( !_extScanner )
      {
         PD_LOG( PDERROR, "Create extent scanner failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsTBScanner::advance( dmsRecordID &recordID,
                                 _mthRecordGenerator &generator,
                                 pmdEDUCB *cb,
                                 _mthMatchTreeContext *mthContext )
   {
      INT32 rc = SDB_OK ;
      if ( _firstRun )
      {
         rc = _firstInit() ;
         PD_RC_CHECK( rc, PDERROR, "First init failed, rc: %d", rc ) ;
      }

      while ( DMS_INVALID_EXTENT != _curExtentID )
      {
         rc = _extScanner->advance( recordID, generator, cb, mthContext ) ;
         if ( SDB_DMS_EOC == rc )
         {
            if ( 0 != _extScanner->getMaxRecords() )
            {
               _curExtentID = _extScanner->nextExtentID() ;
               _resetExtScanner() ;
               _context->pause() ;
               continue ;
            }
            else
            {
               _curExtentID = DMS_INVALID_EXTENT ;
               goto error ;
            }
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Extent scanner failed, rc: %d", rc ) ;
            goto error ;
         }
         else
         {
            goto done ;
         }
      }
      rc = SDB_DMS_EOC ;
      goto error ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsTBScanner::stop()
   {
      _extScanner->stop() ;
      _curExtentID = DMS_INVALID_EXTENT ;
   }

   /*
      _dmsIXSecScanner implement
   */
   _dmsIXSecScanner::_dmsIXSecScanner( dmsStorageDataCommon *su,
                                       dmsMBContext *context,
                                       mthMatchRuntime *matchRuntime,
                                       rtnIXScanner *scanner,
                                       DMS_ACCESS_TYPE accessType,
                                       INT64 maxRecords,
                                       INT64 skipNum,
                                       INT32 flag )
   :_dmsScanner( su, context, matchRuntime, accessType ),
    _curRecordPtr( NULL )
   {
      _maxRecords          = maxRecords ;
      _skipNum             = skipNum ;
      _firstRun            = TRUE ;
      _hasLockedRecord     = FALSE ;
      _pTransCB            = NULL ;
      _recordLock          = DPS_TRANSLOCK_MAX ;
      _needUnLock          = FALSE ;
      _selectForUpdate     = FALSE ;
      _cb                  = NULL ;
      _scanner             = scanner ;
      _onceRestNum         = 0 ;
      _eof                 = FALSE ;
      _indexBlockScan      = FALSE ;
      _judgeStartKey       = FALSE ;
      _includeStartKey     = FALSE ;
      _includeEndKey       = FALSE ;
      _blockScanDir        = 1 ;
      _countOnly           = FALSE ;
      _CSCLLockHeld        = FALSE ;

      if ( OSS_BIT_TEST( flag, FLG_QUERY_FOR_UPDATE ) )
      {
         _selectForUpdate = TRUE ;
      }
   }

   _dmsIXSecScanner::~_dmsIXSecScanner ()
   {
      if ( FALSE == _firstRun && _recordLock != DPS_TRANSLOCK_MAX
           && _hasLockedRecord
           && DMS_INVALID_OFFSET != _curRID._offset )
      {
         _pTransCB->transLockRelease( _cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID, &_callback ) ;
         _hasLockedRecord = FALSE ;
      }

      releaseCSCLLock() ;

      _scanner    = NULL ;
   }

   dmsTransLockCallback* _dmsIXSecScanner::callbackHandler()
   {
      return &_callback ;
   }

   const dmsTransRecordInfo* _dmsIXSecScanner::recordInfo() const
   {
      return _callback.getTransRecordInfo() ;
   }

   void  _dmsIXSecScanner::enableIndexBlockScan( const BSONObj &startKey,
                                                 const BSONObj &endKey,
                                                 const dmsRecordID &startRID,
                                                 const dmsRecordID &endRID,
                                                 INT32 direction )
   {
      SDB_ASSERT( _scanner, "Scanner can't be NULL" ) ;

      _blockScanDir     = direction ;
      _indexBlockScan   = TRUE ;
      _judgeStartKey    = FALSE ;
      _includeStartKey  = FALSE ;
      _includeEndKey    = FALSE ;

      _startKey = startKey.getOwned() ;
      _endKey = endKey.getOwned() ;
      _startRID = startRID ;
      _endRID = endRID ;

      BSONObj startObj = _scanner->getPredicateList()->startKey() ;
      BSONObj endObj = _scanner->getPredicateList()->endKey() ;

      if ( 0 == startObj.woCompare( *_getStartKey(), BSONObj(), false ) &&
           _getStartRID()->isNull() )
      {
         _includeStartKey = TRUE ;
      }
      if ( 0 == endObj.woCompare( *_getEndKey(), BSONObj(), false ) &&
           _getEndRID()->isNull() )
      {
         _includeEndKey = TRUE ;
      }

      // reset start/end RID
      if ( _getStartRID()->isNull() )
      {
         if ( 1 == _scanner->getDirection() )
         {
            _getStartRID()->resetMin () ;
         }
         else
         {
            _getStartRID()->resetMax () ;
         }
      }
      if ( _getEndRID()->isNull() )
      {
         if ( !_includeEndKey )
         {
            if ( 1 == _scanner->getDirection() )
            {
               _getEndRID()->resetMin () ;
            }
            else
            {
               _getEndRID()->resetMax () ;
            }
         }
         else
         {
            if ( 1 == _scanner->getDirection() )
            {
               _getEndRID()->resetMax () ;
            }
            else
            {
               _getEndRID()->resetMin () ;
            }
         }
      }
   }

   INT32 _dmsIXSecScanner::acquireCSCLLock( )
   {
      INT32 rc = SDB_OK ;
      if ( !_CSCLLockHeld && DPS_TRANSLOCK_MAX != _recordLock )
      {
         dpsTransRetInfo   lockConflict ;
         dmsIXTransContext ixTxContext( _context, _accessType,
                                        _scanner ) ;

         if ( DPS_TRANSLOCK_IS == dpsIntentLockMode( _recordLock ) )
         {
            rc = _pTransCB->transLockGetIS( _cb, _pSu->logicalID(),
                                            _context->mbID(),
                                            & ixTxContext, &lockConflict ) ;
         }
         else if ( DPS_TRANSLOCK_IX == dpsIntentLockMode( _recordLock ) )
         {
            rc = _pTransCB->transLockGetIX( _cb, _pSu->logicalID(),
                                            _context->mbID(),
                                             & ixTxContext, &lockConflict ) ;
         }
         else
         {
            goto done ;
         }

         // this is performance improvement, failed to get lock should not
         // fail the operation
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDWARNING,
                      "Failed to get CS/CL lock, rc: %d"OSS_NEWLINE
                      "Conflict ( representative ):"OSS_NEWLINE
                      "   EDUID:  %llu"OSS_NEWLINE
                      "   TID:    %u"OSS_NEWLINE
                      "   LockId: %s"OSS_NEWLINE
                      "   Mode:   %s"OSS_NEWLINE,
                      rc,
                      lockConflict._eduID,
                      lockConflict._tid,
                      lockConflict._lockID.toString().c_str(),
                      lockModeToString( lockConflict._lockType ) ) ;
            goto error ;
         }
         else
         {
            _CSCLLockHeld = TRUE ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   void  _dmsIXSecScanner::releaseCSCLLock( )
   {
      if ( _CSCLLockHeld )
      {
         _pTransCB->transLockRelease( _cb, _pSu->logicalID(),
                                      _context->mbID() ) ;
         _CSCLLockHeld = FALSE ;
      }

   }

   INT32 _dmsIXSecScanner::_firstInit( pmdEDUCB * cb )
   {
      INT32 rc          = SDB_OK ;
      _pTransCB         = pmdGetKRCB()->getTransCB() ;
      dpsTransExecutor *pExe = cb->getTransExecutor() ;

      _transIsolation = pExe->getTransIsolation() ;
      _waitLock = pExe->isTransWaitLock() ;
      _useRollbackSegment = pExe->useRollbackSegment() ;

      /// when not support transaction
      if ( !_pSu->isTransSupport() )
      {
         _recordLock = DPS_TRANSLOCK_MAX ;
         _selectForUpdate = FALSE ;
      }
      /// When not in transaction
      else if ( DPS_INVALID_TRANS_ID == cb->getTransID() )
      {
         /// When not use trans lock
         if ( !pExe->useTransLock() )
         {
            _recordLock = DPS_TRANSLOCK_MAX ;
            _selectForUpdate = FALSE ;
         }
         /// Write operation should release lock right way
         else if ( DMS_IS_WRITE_OPR( _accessType ) )
         {
            _recordLock = DPS_TRANSLOCK_X ;
            _needUnLock = TRUE ;
            _useRollbackSegment = FALSE ;  // don't use old copy
         }
         /// Read is always no lock
         else
         {
            _recordLock = DPS_TRANSLOCK_MAX ;
            _selectForUpdate = FALSE ;
         }
      }
      /// In transaction
      else
      {
         if ( cb->isInTransRollback() )
         {
            _recordLock = DPS_TRANSLOCK_MAX ;
            _selectForUpdate = FALSE ;
         }
         else if ( DMS_IS_WRITE_OPR( _accessType ) )
         {
            _recordLock = DPS_TRANSLOCK_X ;
            _needUnLock = FALSE ;
         }
         else if ( TRANS_ISOLATION_RU == _transIsolation &&
                   !_selectForUpdate )
         {
            _recordLock = DPS_TRANSLOCK_MAX ;
            _selectForUpdate = FALSE ;
         }
         else
         {
            _recordLock = _selectForUpdate ? DPS_TRANSLOCK_U :
                                             DPS_TRANSLOCK_S ;
            if ( TRANS_ISOLATION_RS == _transIsolation ||
                 _selectForUpdate )
            {
               _needUnLock = FALSE ;
               _waitLock = TRUE ;
            }
            else
            {
               _needUnLock = TRUE ;
            }
         }
      }

      if ( NULL == _scanner )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _scanner->setReadonly( isReadOnly() ) ;
      if ( DPS_TRANSLOCK_MAX == _recordLock )
      {
         _scanner->disableByType( SCANNER_TYPE_MEM_TREE ) ;
      }
      if ( cb && cb->isInterrupted() )
      {
         rc = SDB_APP_INTERRUPT ;
         goto error ;
      }
      if ( !_context->isMBLock( _mbLockType ) )
      {
         rc = _context->mbLock( _mbLockType ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb lock failed, rc: %d", rc ) ;
      }
      if ( !dmsAccessAndFlagCompatiblity ( _context->mb()->_flag,
                                           _accessType ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  _context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      rc = _scanner->resumeScan() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resum ixscan, rc: %d", rc ) ;
      _cb   = cb ;

      // As a performance improvement, we are going to acquire the CS and
      // CL lock right in the beginning to avoid extra performance overhead
      // to acquire these locks when acquiring record lock in each step
      // We release and require the lock during pauseScan/resumeScan
      rc = acquireCSCLLock() ;
      if ( rc )
      {
         goto error ;
      }

      /// set callback info
      _callback.setBaseInfo( _pTransCB, cb ) ;
      _callback.setIDInfo( _pSu->CSID(), _context->mbID(),
                           _pSu->logicalID(),
                           _context->clLID() ) ;
      _callback.setIXScanner( _scanner ) ;

      // unset first run
      _firstRun = FALSE ;
      _onceRestNum = (INT64)pmdGetKRCB()->getOptionCB()->indexScanStep() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   BSONObj* _dmsIXSecScanner::_getStartKey ()
   {
      return _scanner->getDirection() == _blockScanDir ? &_startKey : &_endKey ;
   }

   BSONObj* _dmsIXSecScanner::_getEndKey ()
   {
      return _scanner->getDirection() == _blockScanDir ? &_endKey : &_startKey ;
   }

   dmsRecordID* _dmsIXSecScanner::_getStartRID ()
   {
      return _scanner->getDirection() == _blockScanDir ? &_startRID : &_endRID ;
   }

   dmsRecordID* _dmsIXSecScanner::_getEndRID ()
   {
      return _scanner->getDirection() == _blockScanDir ? &_endRID : &_startRID ;
   }

   void _dmsIXSecScanner::_updateMaxRecordsNum( _mthRecordGenerator &generator )
   {
      if ( _maxRecords > 0 )
      {
         if ( _maxRecords >= generator.getRecordNum() )
         {
            _maxRecords -= generator.getRecordNum() ;
         }
         else
         {
            INT32 num = generator.getRecordNum() - _maxRecords ;
            generator.popTail( num ) ;
            _maxRecords = 0 ;
         }
      }
   }

   // Description
   //    Index section scan. Advance to next index entry based on the on-disk
   // index tree and searching criteria, return the found recordID pointed
   // by the index
   // Input
   //
   // Ouput
   //    recordID:
   //    On normal return, record lock is held on the matching record.
   // Return
   //
   //
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIXSECSCAN_ADVANCE, "_dmsIXSecScanner::advance" )
   INT32 _dmsIXSecScanner::advance( dmsRecordID &recordID,
                                    _mthRecordGenerator &generator,
                                    pmdEDUCB * cb,
                                    _mthMatchTreeContext *mthContext )
   {
      INT32 rc                = SDB_OK ;
      BOOLEAN result          = TRUE ;
      ossValuePtr recordDataPtr ;
      dmsRecordData recordData ;
      dmsRecordID waitUnlockRID ;
      BOOLEAN ignoredLock     = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSIXSECSCAN_ADVANCE );

      PD_TRACE5( SDB__DMSIXSECSCAN_ADVANCE,
                 PD_PACK_UINT(_needUnLock),
                 PD_PACK_UINT(_mbLockType),
                 PD_PACK_UINT(_accessType),
                 PD_PACK_UINT(_waitLock),
                 PD_PACK_UINT(_recordLock) );

      if ( _firstRun )
      {
         rc = _firstInit( cb ) ;
         PD_RC_CHECK( rc, PDWARNING, "first init failed, rc: %d", rc ) ;
      }
      // last run have record lock held, but not trans, need to release
      // record lock
      else if ( _needUnLock && _hasLockedRecord &&
                DMS_INVALID_OFFSET != _curRID._offset )
      {
         _pTransCB->transLockRelease( cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID, &_callback ) ;
         _hasLockedRecord = FALSE ;
      }

      _hasLockedRecord = FALSE ;
      while ( _onceRestNum-- > 0 && 0 != _maxRecords )
      {
         ignoredLock = FALSE ;
         // advance index tree
         rc = _scanner->advance( _curRID ) ;
         if ( SDB_IXM_EOC == rc )
         {
            _eof = TRUE ;
            rc = SDB_DMS_EOC ;
            break ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "IXScanner advance failed, rc: %d", rc ) ;
            goto error ;
         }
         SDB_ASSERT( _curRID.isValid(), "rid must be valid" ) ;

         // index block scan
         if ( _indexBlockScan )
         {
            INT32 result = 0 ;
            if ( !_judgeStartKey )
            {
               _judgeStartKey = TRUE ;

               result = _scanner->compareWithCurKeyObj( *_getStartKey() ) ;
               if ( 0 == result )
               {
                  result = _curRID.compare( *_getStartRID() ) *
                           _scanner->getDirection() ;
               }
               if ( result < 0 )
               {
                  // need to relocate
                  rc = _scanner->relocateRID( *_getStartKey(),
                                              *_getStartRID() ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to relocateRID, rc: %d",
                               rc ) ;
                  continue ;
               }
            }

            result = _scanner->compareWithCurKeyObj( *_getEndKey() ) ;
            if ( 0 == result )
            {
               result = _curRID.compare( *_getEndRID() ) *
                        _scanner->getDirection() ;
            }
            if ( result > 0 || ( !_includeEndKey && result == 0 ) )
            {
               _eof = TRUE ;
               rc = SDB_DMS_EOC ;
               break ;
            }
         }

         // don't read data
         if ( !_matchRuntime )
         {
            if ( _skipNum > 0 )
            {
               --_skipNum ;
               continue ;
            }
            else if ( _countOnly )
            {
               if ( _maxRecords > 0 )
               {
                  --_maxRecords ;
               }
               recordID = _curRID ;
               recordDataPtr = 0 ;
               generator.setDataPtr( recordDataPtr ) ;
               goto done ;
            }
         }

         // record2RW already take care of in memory version vs on disk
         // version under the cover
         _recordRW = _pSu->record2RW( _curRID, _context->mbID() ) ;

         // If need lock and the RID was not setup from the in memory tree.
         // As an optimization, if the RID was originally found through in
         // memory tree scan, don't try lock at all.
         if ( _recordLock != DPS_TRANSLOCK_MAX )
         {
            dpsTransRetInfo   lockConflict ;
            dmsIXTransContext ixTxContext( _context, _accessType, _scanner ) ;

            /// already locked, but not the same, should release lock first
            if ( waitUnlockRID.isValid() && _curRID != waitUnlockRID )
            {
               _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                            _context->mbID(), &waitUnlockRID,
                                            &_callback ) ;
               waitUnlockRID.reset() ;
            }

            // attach the recordRW in callback
            _callback.attachRecordRW( &_recordRW ) ;
            _callback.clearStatus() ;

            if ( DPS_TRANSLOCK_X == _recordLock )
            {
               // exclusive lock has to always wait on the lock
               rc = _pTransCB->transLockGetX( cb, _pSu->logicalID(),
                                              _context->mbID(), &_curRID,
                                              &ixTxContext,
                                              &lockConflict, &_callback ) ;
            }
            else if ( DPS_TRANSLOCK_U == _recordLock )
            {
               rc = _pTransCB->transLockGetU( cb, _pSu->logicalID(),
                                              _context->mbID(), &_curRID,
                                              &ixTxContext,
                                              &lockConflict, &_callback ) ;
            }
            // DPS_TRANSLOCK_S
            else
            {
               if ( !needWaitForLock() )
               {
                  // for new RC logic, we should first try on S lock instead
                  // of directly wait on the record lock. Under the cover,
                  // the lock call back function would try to use the old copy
                  // (previous committed version) if exist
                  rc = _pTransCB->transLockTestSPreempt( cb, _pSu->logicalID(),
                                                         _context->mbID(),
                                                         &_curRID,
                                                         &lockConflict,
                                                         &_callback ) ;
                  ignoredLock = TRUE ;
                  if ( _callback.isSkipRecord() )
                  {
                     // For newly created records by another transaction,
                     // we could still find it through diskIXScan, we will
                     // skip those records without waiting for lock.
                     _scanner->removeDuplicatRID( _curRID ) ;
                     rc = SDB_OK ;
                     continue ;
                  }
                  if ( _callback.isUseOldVersion() )
                  {
                     rc = SDB_OK ;
                  }
               }

               /// wait lock
               if ( needWaitForLock() || rc )
               {
                  rc = _pTransCB->transLockGetS( cb, _pSu->logicalID(),
                                                 _context->mbID(), &_curRID,
                                                 &ixTxContext,
                                                 &lockConflict,
                                                 &_callback ) ;
                  if ( SDB_OK == rc )
                  {
                     ignoredLock = FALSE ;
                  }
               }
            }

            if ( waitUnlockRID.isValid() )
            {
               _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                            _context->mbID(), &waitUnlockRID,
                                            &_callback ) ;
               waitUnlockRID.reset() ;
            }

            if ( rc )
            {
               PD_LOG( PDERROR,
                       "Failed to get record lock, rc: %d"OSS_NEWLINE
                       "Request Mode:   %s"OSS_NEWLINE
                       "Conflict ( representative ):"OSS_NEWLINE
                       "   EDUID:  %llu"OSS_NEWLINE
                       "   TID:    %u"OSS_NEWLINE
                       "   LockId: %s"OSS_NEWLINE
                       "   Mode:   %s"OSS_NEWLINE,
                       rc,
                       lockModeToString( _recordLock ),
                       lockConflict._eduID,
                       lockConflict._tid,
                       lockConflict._lockID.toString().c_str(),
                       lockModeToString( lockConflict._lockType ) ) ;
               cb->printInfo( EDU_INFO_ERROR, "Failed to get record lock" ) ;
               goto error ;
            }

            if ( !ignoredLock )
            {
               _hasLockedRecord = TRUE ;
            }

            if ( _callback.hasError() )
            {
               rc = _callback.getResult() ;
               PD_LOG( PDERROR, "Occur error in callback, rc: %d", rc ) ;
               goto error ;
            }

            // index has changed under us between wait on lock and got lock
            if ( !ixTxContext.isCursorSame() || _callback.isSkipRecord() )
            {
               if ( _hasLockedRecord )
               {
                  waitUnlockRID = _curRID ;
                  _hasLockedRecord = FALSE ;
               }

               /// remove the duplicate key
               _scanner->removeDuplicatRID( _curRID ) ;

#ifdef _DEBUG
               PD_LOG( PDDEBUG, "Cursor changed while waiting for lock, "
                       "rid(%d, %d), isCursorSame(%d), _onceRestNum(%d), "
                       "isSkipRecord(%d)",
                       _curRID._extent, _curRID._offset,
                       ixTxContext.isCursorSame(), _onceRestNum,
                       _callback.isSkipRecord()) ;
#endif
               // When cursor changed, we may need to go back to previous
               // key to retry, don't count as a step. Also avoid potential
               // pause here if step becomes 0, in which case we may unexpectly
               // lose previously savedObj and savedRID and cause skip record.
               if ( !ixTxContext.isCursorSame() )
               {
                  _onceRestNum++ ;
               }
               continue ;
            }
         } // end of (_recordLock != -1)

         // Move _curRecordPtr to here so that _recordRW is fully setup for
         // all cases.
         _curRecordPtr = _recordRW.readPtr( 0 ) ;

         // Handle the record being deleted
         if ( _curRecordPtr->isDeleting() )
         {
            if ( _recordLock == DPS_TRANSLOCK_X )
            {
               rc = _pSu->deleteRecord( _context, _curRID, 0,
                                        cb, NULL, NULL,
                                        _callback.getTransRecordInfo() ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDWARNING, "Failed to delete the deleting record, "
                          "rc: %d", rc ) ;
               }
            }

            if ( _hasLockedRecord )
            {
               _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                            _context->mbID(), &_curRID,
                                            &_callback ) ;
               _hasLockedRecord = FALSE ;
            }

            /// remove the duplicate key
            _scanner->removeDuplicatRID( _curRID ) ;

            continue ;
         }
         SDB_ASSERT( !_curRecordPtr->isDeleted(),
                     "record can't be deleted" ) ;

         recordID = _curRID ;
         rc = _pSu->extractData( _context, _recordRW, cb, recordData ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Extract record data failed, rc: %d", rc ) ;
            goto error ;
         }
         recordDataPtr = ( ossValuePtr )recordData.data() ;
         generator.setDataPtr( recordDataPtr ) ;

         // match
         if ( _matchRuntime && _matchRuntime->getMatchTree() )
         {
            result = TRUE ;
            try
            {
               _mthMatchTree *matcher = _matchRuntime->getMatchTree() ;
               rtnParamList *parameters = _matchRuntime->getParametersPointer() ;
               BSONObj obj ( recordData.data() ) ;
               //do not clear dollarlist flag
               mthContextClearRecordInfoSafe( mthContext ) ;
               rc = matcher->matches( obj, result, mthContext, parameters ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Failed to match record, rc: %d", rc ) ;
                  goto error ;
               }
               if ( result )
               {
                  rc = generator.resetValue( obj, mthContext ) ;
                  PD_RC_CHECK( rc, PDERROR, "resetValue failed:rc=%d", rc ) ;
                  if ( _skipNum > 0 )
                  {
                     if ( _skipNum >= generator.getRecordNum() )
                     {
                        _skipNum -= generator.getRecordNum() ;
                     }
                     else
                     {
                        generator.popFront( _skipNum ) ;
                        _skipNum = 0 ;
                        _updateMaxRecordsNum( generator ) ;
                        goto done ;
                     }
                  }
                  else
                  {
                     _updateMaxRecordsNum( generator ) ;
                     goto done ; // find ok
                  }
               }
            }
            catch( std::exception &e )
            {
               PD_LOG ( PDERROR, "Failed to create BSON object: %s",
                        e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         } // if ( _match )
         else
         {
            try
            {
               BSONObj obj( recordData.data() ) ;
               rc = generator.resetValue( obj, mthContext ) ;
               PD_RC_CHECK( rc, PDERROR, "resetValue failed:rc=%d", rc ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_RC_CHECK( rc, PDERROR, "Failed to create BSON object: %s",
                            e.what() ) ;
               goto error ;
            }

            if ( _skipNum > 0 )
            {
               --_skipNum ;
            }
            else
            {
               if ( _maxRecords > 0 )
               {
                  --_maxRecords ;
               }
               goto done ; // find ok
            }
         }

         // Proper found case will jump to done, if we got here, there was
         // either unmatch, or we need to skip. Either way, we should
         // release the record lock before advance to next index
         if ( _hasLockedRecord )
         {
            _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                         _context->mbID(), &_curRID,
                                         &_callback ) ;
            _hasLockedRecord = FALSE ;
         }
      } // while

      rc = SDB_DMS_EOC ;
      {
         INT32 rcTmp = _scanner->pauseScan() ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Pause scan failed, rc: %d", rcTmp ) ;
            rc = rcTmp ;
         }
         // release CS/CL lock when we are done
         releaseCSCLLock() ;
      }
      goto error ;

   done:
      if ( waitUnlockRID.isValid() )
      {
         _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                      _context->mbID(), &waitUnlockRID,
                                      &_callback ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSIXSECSCAN_ADVANCE, rc ) ;
      return rc ;
   error:
      if ( _hasLockedRecord && _recordLock != DPS_TRANSLOCK_MAX )
      {
         _pTransCB->transLockRelease( cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID, &_callback ) ;
         _hasLockedRecord = FALSE ;
      }
      releaseCSCLLock() ;
      recordID.reset() ;
      recordDataPtr = 0 ;
      generator.setDataPtr( recordDataPtr ) ;
      _curRID._offset = DMS_INVALID_OFFSET ;
      goto done ;
   }

   void _dmsIXSecScanner::stop ()
   {
      if ( FALSE == _firstRun && _recordLock != DPS_TRANSLOCK_MAX
           && _hasLockedRecord &&
           DMS_INVALID_OFFSET != _curRID._offset )
      {
         _pTransCB->transLockRelease( _cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID, &_callback ) ;
         _hasLockedRecord = FALSE ;
      }
      if ( DMS_INVALID_OFFSET != _curRID._offset )
      {
         INT32 rc = _scanner->pauseScan() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Pause scan failed, rc: %d", rc ) ;
         }
      }
      releaseCSCLLock() ;
      _curRID._offset = DMS_INVALID_OFFSET ;
   }

   /*
      _dmsIXScanner implement
   */
   _dmsIXScanner::_dmsIXScanner( dmsStorageDataCommon *su,
                                 dmsMBContext *context,
                                 mthMatchRuntime *matchRuntime,
                                 rtnIXScanner *scanner,
                                 BOOLEAN ownedScanner,
                                 DMS_ACCESS_TYPE accessType,
                                 INT64 maxRecords,
                                 INT64 skipNum,
                                 INT32 flag )
   :_dmsScanner( su, context, matchRuntime, accessType ),
    _secScanner( su, context, matchRuntime, scanner, accessType, maxRecords,
                 skipNum, flag )
   {
      _scanner       = scanner ;
      _eof           = FALSE ;
      _ownedScanner  = ownedScanner ;
   }

   _dmsIXScanner::~_dmsIXScanner()
   {
      if ( _scanner && _ownedScanner )
      {
         SDB_OSS_DEL _scanner ;
      }
      _scanner       = NULL ;
   }

   dmsTransLockCallback* _dmsIXScanner::callbackHandler()
   {
      return _secScanner.callbackHandler() ;
   }

   const dmsTransRecordInfo* _dmsIXScanner::recordInfo() const
   {
      return _secScanner.recordInfo() ;
   }

   void _dmsIXScanner::_resetIXSecScanner ()
   {
      _secScanner._firstRun = TRUE ;
   }

   INT32 _dmsIXScanner::advance( dmsRecordID &recordID,
                                 _mthRecordGenerator &generator,
                                 pmdEDUCB * cb,
                                 _mthMatchTreeContext *mthContext )
   {
      INT32 rc = SDB_OK ;

      while ( !_eof )
      {
         rc = _secScanner.advance( recordID, generator, cb, mthContext ) ;
         if ( SDB_DMS_EOC == rc )
         {
            if ( 0 != _secScanner.getMaxRecords() &&
                 !_secScanner.eof() )
            {
               _resetIXSecScanner() ;
               _context->pause() ;
               continue ;
            }
            else
            {
               _eof = TRUE ;
               goto error ;
            }
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "IX scanner failed, rc: %d", rc ) ;
            goto error ;
         }
         else
         {
            goto done ;
         }
      }
      rc = SDB_DMS_EOC ;
      goto error ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsIXScanner::stop ()
   {
      _secScanner.stop() ;
      _eof = TRUE ;
   }

   /*
      _dmsExtentItr implement
   */
   _dmsExtentItr::_dmsExtentItr( dmsStorageData *su, dmsMBContext * context,
                                 DMS_ACCESS_TYPE accessType,
                                 INT32 direction )
   :_curExtent( NULL )
   {
      SDB_ASSERT( su, "storage data unit can't be NULL" ) ;
      SDB_ASSERT( context, "context can't be NULL" ) ;

      _pSu = su ;
      _context = context ;
      _extentCount = 0 ;
      _accessType = accessType ;
      _direction = direction ;
   }

   void _dmsExtentItr::reset( INT32 direction )
   {
      _extentCount = 0 ;
      _direction = direction ;
      _curExtent = NULL ;
   }

   _dmsExtentItr::~_dmsExtentItr ()
   {
      _pSu = NULL ;
      _context = NULL ;
      _curExtent = NULL ;
   }

   #define DMS_EXTENT_ITR_EXT_PERCOUNT    20

   INT32 _dmsExtentItr::next( dmsExtentID &extentID, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( _extentCount >= DMS_EXTENT_ITR_EXT_PERCOUNT )
      {
         _context->pause() ;
         _extentCount = 0 ;

         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }
      }

      if ( !_context->isMBLock() )
      {
         if ( _context->canResume() )
         {
            rc = _context->resume() ;
         }
         else
         {
            rc = _context->mbLock( DMS_IS_WRITE_OPR( _accessType ) ?
                                   EXCLUSIVE : SHARED ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

         if ( !dmsAccessAndFlagCompatiblity ( _context->mb()->_flag,
                                              _accessType ) )
         {
            PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                     _context->mb()->_flag ) ;
            rc = SDB_DMS_INCOMPATIBLE_MODE ;
            goto error ;
         }
      }

      if ( NULL == _curExtent )
      {
         if ( _direction > 0 )
         {
            if ( DMS_INVALID_EXTENT == _context->mb()->_firstExtentID )
            {
               rc = SDB_DMS_EOC ;
               goto error ;
            }
            else
            {
               _extRW = _pSu->extent2RW( _context->mb()->_firstExtentID,
                                         _context->mbID() ) ;
               _extRW.setNothrow( FALSE ) ;
               _curExtent = _extRW.readPtr<dmsExtent>() ;
            }
         }
         else
         {
            if ( DMS_INVALID_EXTENT == _context->mb()->_lastExtentID )
            {
               rc = SDB_DMS_EOC ;
               goto error ;
            }
            else
            {
               _extRW = _pSu->extent2RW( _context->mb()->_lastExtentID,
                                         _context->mbID() ) ;
               _extRW.setNothrow( FALSE ) ;
               _curExtent = _extRW.readPtr<dmsExtent>() ;
            }
         }
      }
      else if ( ( _direction >= 0 &&
                  DMS_INVALID_EXTENT == _curExtent->_nextExtent ) ||
                ( _direction < 0 &&
                  DMS_INVALID_EXTENT == _curExtent->_prevExtent ) )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      else
      {
         dmsExtentID next = _direction >= 0 ?
                            _curExtent->_nextExtent :
                            _curExtent->_prevExtent ;
         _extRW = _pSu->extent2RW( next, _context->mbID() ) ;
         _extRW.setNothrow( FALSE ) ;
         _curExtent = _extRW.readPtr<dmsExtent>() ;
      }

      if ( 0 == _curExtent )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !_curExtent->validate( _context->mbID() ) )
      {
         PD_LOG( PDERROR, "Invalid extent[%d]", _extRW.getExtentID() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      extentID = _extRW.getExtentID() ;
      ++_extentCount ;

   done:
      return rc ;
   error:
      goto done ;
   }

   _dmsExtScannerFactory::_dmsExtScannerFactory()
   {
   }

   _dmsExtScannerFactory::~_dmsExtScannerFactory()
   {
   }

   dmsExtScannerBase* _dmsExtScannerFactory::create( dmsStorageDataCommon *su,
                                                     dmsMBContext *context,
                                                     mthMatchRuntime *matchRuntime,
                                                     dmsExtentID curExtentID,
                                                     DMS_ACCESS_TYPE accessType,
                                                     INT64 maxRecords,
                                                     INT64 skipNum,
                                                     INT32 flag )
   {
      dmsExtScannerBase* scanner = NULL ;

      if ( OSS_BIT_TEST( DMS_MB_ATTR_CAPPED, context->mb()->_attributes ) )
      {
         scanner = SDB_OSS_NEW dmsCappedExtScanner( su, context,
                                                    matchRuntime,
                                                    curExtentID,
                                                    accessType,
                                                    maxRecords,
                                                    skipNum,
                                                    flag ) ;
      }
      else
      {
         scanner = SDB_OSS_NEW dmsExtScanner( su, context,
                                              matchRuntime,
                                              curExtentID,
                                              accessType,
                                              maxRecords,
                                              skipNum,
                                              flag ) ;
      }

      if ( !scanner )
      {
         PD_LOG( PDERROR, "Allocate memory for extent scanner failed" ) ;
         goto error ;
      }

   done:
      return scanner ;
   error:
      goto done ;
   }

   dmsExtScannerFactory* dmsGetScannerFactory()
   {
      static dmsExtScannerFactory s_factory ;
      return &s_factory ;
   }


}


