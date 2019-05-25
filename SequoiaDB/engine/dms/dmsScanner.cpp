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

   Source File Name = dmsTBScanner.hpp

   Descriptive Name = Data Management Service Storage Unit Header

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
#include "rtnIXScanner.hpp"
#include "bpsPrefetch.hpp"
#include "dmsCompress.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"

using namespace bson ;

namespace engine
{

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
                                           INT64 maxRecords, INT64 skipNum )
   :_dmsScanner( su, context, matchRuntime, accessType ),
    _curRecordPtr( NULL )
   {
      _maxRecords          = maxRecords ;
      _skipNum             = skipNum ;
      _next                = DMS_INVALID_OFFSET ;
      _firstRun            = TRUE ;
      _extent              = NULL ;
      _pTransCB            = NULL ;
      _curRID._extent      = curExtentID ;
      _recordXLock         = FALSE ;
      _needUnLock          = FALSE ;
      _cb                  = NULL ;

      if ( DMS_ACCESS_TYPE_UPDATE == _accessType ||
           DMS_ACCESS_TYPE_DELETE == _accessType ||
           DMS_ACCESS_TYPE_INSERT == _accessType )
      {
         _recordXLock = TRUE ;
      }
   }

   _dmsExtScannerBase::~_dmsExtScannerBase ()
   {
      _extent     = NULL ;

      if ( FALSE == _firstRun && _recordXLock &&
           DMS_INVALID_OFFSET != _curRID._offset )
      {
         _pTransCB->transLockRelease( _cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID ) ;
      }
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
      else if ( _needUnLock && DMS_INVALID_OFFSET != _curRID._offset )
      {
         _pTransCB->transLockRelease( cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID ) ;
      }

      rc = _fetchNext( recordID, generator, cb, mthContext ) ;
      if ( rc )
      {
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
      if ( FALSE == _firstRun && _recordXLock &&
           DMS_INVALID_OFFSET != _curRID._offset )
      {
         _pTransCB->transLockRelease( _cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID ) ;
      }
      _next = DMS_INVALID_OFFSET ;
      _curRID._offset = DMS_INVALID_OFFSET ;
   }

   _dmsExtScanner::_dmsExtScanner( dmsStorageDataCommon *su, _dmsMBContext *context,
                                   mthMatchRuntime *matchRuntime,
                                   dmsExtentID curExtentID,
                                   DMS_ACCESS_TYPE accessType,
                                   INT64 maxRecords, INT64 skipNum )
   : _dmsExtScannerBase( su, context, matchRuntime, curExtentID, accessType,
                         maxRecords, skipNum )
   {
   }

   _dmsExtScanner::~_dmsExtScanner()
   {
   }

   INT32 _dmsExtScanner::_firstInit( pmdEDUCB *cb )
   {
      INT32 rc          = SDB_OK ;
      _pTransCB         = pmdGetKRCB()->getTransCB() ;
      SDB_BPSCB *pBPSCB = pmdGetKRCB()->getBPSCB () ;
      BOOLEAN   bPreLoadEnabled = pBPSCB->isPreLoadEnabled() ;
      INT32 lockType    = _recordXLock ? EXCLUSIVE : SHARED ;

      if ( _recordXLock && DPS_INVALID_TRANS_ID == cb->getTransID() )
      {
         _needUnLock = TRUE ;
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
      if ( !_context->isMBLock( lockType ) )
      {
         rc = _context->mbLock( lockType ) ;
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

      if ( bPreLoadEnabled && DMS_INVALID_EXTENT != _extent->_nextExtent )
      {
         pBPSCB->sendPreLoadRequest ( bpsPreLoadReq( _pSu->CSID(),
                                                     _pSu->logicalID(),
                                                     _extent->_nextExtent )) ;
      }

      _cb   = cb ;
      _next = _extent->_firstRecordOffset ;

      _firstRun = FALSE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsExtScanner::_fetchNext( dmsRecordID &recordID,
                                     _mthRecordGenerator &generator,
                                     pmdEDUCB *cb,
                                     _mthMatchTreeContext *mthContext )
   {
      INT32 rc                = SDB_OK ;
      BOOLEAN result          = TRUE ;
      ossValuePtr recordDataPtr ;
      dmsRecordData recordData ;
      BOOLEAN lockedRecord    = FALSE ;

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

         if ( _recordXLock )
         {
            rc = _pTransCB->tryOrAppendX( cb, _pSu->logicalID(),
                                          _context->mbID(), &_curRID ) ;
            if ( rc )
            {
               PD_CHECK( SDB_DPS_TRANS_APPEND_TO_WAIT == rc, rc, error, PDERROR,
                         "Failed to get record, append lock-wait-queue failed, "
                         "rc: %d", rc ) ;
               _context->pause() ;
               {
               DPS_TRANS_WAIT_LOCK _transWaitLock( cb, _pSu->logicalID(),
                                                   _context->mbID(), &_curRID ) ;
               rc = _pTransCB->waitLock( cb, _pSu->logicalID(),
                                         _context->mbID(), &_curRID ) ;
               }
               PD_RC_CHECK( rc, PDERROR, "Failed to wait record lock, rc: %d",
                            rc ) ;
               lockedRecord = TRUE ;
               rc = _context->resume() ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Remove dms mb context failed, rc: %d", rc );
                  goto error ;
               }
               if ( !dmsAccessAndFlagCompatiblity ( _context->mb()->_flag,
                                                    _accessType ) )
               {
                  PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                           _context->mb()->_flag ) ;
                  rc = SDB_DMS_INCOMPATIBLE_MODE ;
                  goto error ;
               }
               _next = _curRecordPtr->getNextOffset() ;
            }
         }

         if ( _curRecordPtr->isDeleting() )
         {
            if ( _recordXLock )
            {
               INT32 rc1 = _pSu->deleteRecord( _context, _curRID,
                                               0, cb, NULL ) ;
               if ( rc1 )
               {
                  PD_LOG( PDWARNING, "Failed to delete the deleting record, "
                          "rc: %d", rc ) ;
               }
               _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                            _context->mbID(), &_curRID ) ;
               lockedRecord = FALSE ;
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

            if ( _matchRuntime && _matchRuntime->getMatchTree() )
            {
               result = TRUE ;
               try
               {
                  _mthMatchTree *matcher = _matchRuntime->getMatchTree() ;
                  rtnParamList *parameters = _matchRuntime->getParametersPointer() ;
                  BSONObj obj( recordData.data() ) ;
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

         if ( _recordXLock )
         {
            _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                         _context->mbID(), &_curRID ) ;
            lockedRecord = FALSE ;
         }
      } // while

      rc = SDB_DMS_EOC ;
      goto error ;

   done:
      return rc ;
   error:
      if ( lockedRecord && _recordXLock )
      {
         _pTransCB->transLockRelease( cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID ) ;
      }
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
                                               INT64 skipNum )
   : _dmsExtScannerBase( su, context, matchRuntime, curExtentID, accessType,
                         maxRecords, skipNum )
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
   }

   _dmsCappedExtScanner::~_dmsCappedExtScanner()
   {
   }

   INT32 _dmsCappedExtScanner::_firstInit( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      _pTransCB = pmdGetKRCB()->getTransCB() ;
      INT32 lockType = _recordXLock ? EXCLUSIVE : SHARED ;
      BOOLEAN inRange = FALSE ;

      if ( _recordXLock && DPS_INVALID_TRANS_ID == cb->getTransID() )
      {
         _needUnLock = TRUE ;
      }

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

      if ( !_context->isMBLock( lockType ) )
      {
         rc = _context->mbLock( lockType ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb lock failed, rc: %d", rc ) ;
      }

      _workExtInfo =
         (( _dmsStorageDataCapped * )_pSu)->getWorkExtInfo( _context->mbID() ) ;

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
            BSONObj boStartKey = BSON( DMS_ID_KEY_NAME << 0 ) ;
            BSONObj boStopKey = BSON( DMS_ID_KEY_NAME << OSS_SINT64_MAX ) ;

            rc = predicateSet.addPredicate( DMS_ID_KEY_NAME,
                                            boStartKey.firstElement(),
                                            BSONObj::GTE, FALSE, FALSE, FALSE,
                                            -1, -1 ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add predicate, rc: %d", rc ) ;
            rc = predicateSet.addPredicate( DMS_ID_KEY_NAME,
                                            boStopKey.firstElement(),
                                            BSONObj::LTE, FALSE, FALSE, FALSE,
                                            -1, -1 ) ;
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

   INT32 _dmsCappedExtScanner::_validateRange( BOOLEAN &inRange )
   {
      INT32 rc = SDB_OK ;

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
                                 INT64 maxRecords, INT64 skipNum )
   :_dmsScanner( su, context, matchRuntime, accessType )
   {
      _extScanner    = NULL ;
      _curExtentID   = DMS_INVALID_EXTENT ;
      _firstRun      = TRUE ;
      _maxRecords    = maxRecords ;
      _skipNum       = skipNum ;
   }

   _dmsTBScanner::~_dmsTBScanner()
   {
      _curExtentID   = DMS_INVALID_EXTENT ;
      if ( _extScanner )
      {
         SDB_OSS_DEL _extScanner ;
      }
   }

   INT32 _dmsTBScanner::_firstInit()
   {
      INT32 rc = SDB_OK ;
      INT32 lockType = SHARED ;

      rc = _getExtScanner() ;
      PD_RC_CHECK( rc, PDERROR, "Get extent scanner failed, rc: %d", rc ) ;

      lockType = _extScanner->_recordXLock ? EXCLUSIVE : SHARED ;

      if ( !_context->isMBLock( lockType ) )
      {
         rc = _context->mbLock( lockType ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb lock failed, rc: %d", rc ) ;
      }
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
   }

   INT32 _dmsTBScanner::_getExtScanner()
   {
      INT32 rc = SDB_OK ;
      _extScanner = dmsGetScannerFactory()->create( _pSu, _context,
                                                    _matchRuntime,
                                                    _curExtentID,
                                                    _accessType,
                                                    _maxRecords,
                                                    _skipNum ) ;
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
                                       INT64 skipNum )
   :_dmsScanner( su, context, matchRuntime, accessType ),
    _curRecordPtr( NULL )
   {
      _maxRecords          = maxRecords ;
      _skipNum             = skipNum ;
      _firstRun            = TRUE ;
      _pTransCB            = NULL ;
      _recordXLock         = FALSE ;
      _needUnLock          = FALSE ;
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

      if ( DMS_ACCESS_TYPE_UPDATE == _accessType ||
           DMS_ACCESS_TYPE_DELETE == _accessType ||
           DMS_ACCESS_TYPE_INSERT == _accessType )
      {
         _recordXLock = TRUE ;
      }
   }

   _dmsIXSecScanner::~_dmsIXSecScanner ()
   {
      if ( FALSE == _firstRun && _recordXLock &&
           DMS_INVALID_OFFSET != _curRID._offset )
      {
         _pTransCB->transLockRelease( _cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID ) ;
      }

      _scanner    = NULL ;
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

   INT32 _dmsIXSecScanner::_firstInit( pmdEDUCB * cb )
   {
      INT32 rc          = SDB_OK ;
      _pTransCB         = pmdGetKRCB()->getTransCB() ;
      INT32 lockType    = SHARED ;

      if ( _countOnly )
      {
         _recordXLock = FALSE ;
      }
      if ( _recordXLock )
      {
         lockType = EXCLUSIVE ;
      }
      if ( _recordXLock && DPS_INVALID_TRANS_ID == cb->getTransID() )
      {
         _needUnLock = TRUE ;
      }
      if ( NULL == _scanner )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( cb && cb->isInterrupted() )
      {
         rc = SDB_APP_INTERRUPT ;
         goto error ;
      }
      if ( !_context->isMBLock( lockType ) )
      {
         rc = _context->mbLock( lockType ) ;
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

      rc = _scanner->resumeScan( _recordXLock ? FALSE : TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resum ixscan, rc: %d", rc ) ;

      _cb   = cb ;

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

   INT32 _dmsIXSecScanner::advance( dmsRecordID &recordID,
                                    _mthRecordGenerator &generator,
                                    pmdEDUCB * cb,
                                    _mthMatchTreeContext *mthContext )
   {
      INT32 rc                = SDB_OK ;
      BOOLEAN result          = TRUE ;
      ossValuePtr recordDataPtr ;
      dmsRecordData recordData ;
      BOOLEAN lockedRecord    = FALSE ;

      if ( _firstRun )
      {
         rc = _firstInit( cb ) ;
         PD_RC_CHECK( rc, PDWARNING, "first init failed, rc: %d", rc ) ;
      }
      else if ( _needUnLock && DMS_INVALID_OFFSET != _curRID._offset )
      {
         _pTransCB->transLockRelease( cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID ) ;
      }

      while ( _onceRestNum-- > 0 && 0 != _maxRecords )
      {
         rc = _scanner->advance( _curRID, _recordXLock ? FALSE : TRUE ) ;
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
         SDB_ASSERT( _curRID.isValid(), "rid msut valid" ) ;

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

         _recordRW = _pSu->record2RW( _curRID, _context->mbID() ) ;
         _curRecordPtr = _recordRW.readPtr( 0 ) ;
         if ( _recordXLock )
         {
            rc = _pTransCB->tryOrAppendX( cb, _pSu->logicalID(),
                                          _context->mbID(), &_curRID ) ;
            if ( rc )
            {
               PD_CHECK( SDB_DPS_TRANS_APPEND_TO_WAIT == rc, rc, error, PDERROR,
                         "Failed to get record, append lock-wait-queue failed, "
                         "rc: %d", rc ) ;
               rc = _scanner->pauseScan( _recordXLock ? FALSE : TRUE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to pause ixscan, rc: %d", rc ) ;

               _context->pause() ;
               {

               DPS_TRANS_WAIT_LOCK _transWaitLock( cb, _pSu->logicalID(),
                                                   _context->mbID(), &_curRID ) ;
               rc = _pTransCB->waitLock( cb, _pSu->logicalID(),
                                         _context->mbID(), &_curRID ) ;
               }
               PD_RC_CHECK( rc, PDERROR, "Failed to wait record lock, rc: %d",
                            rc ) ;
               lockedRecord = TRUE ;
               rc = _context->resume() ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Remove dms mb context failed, rc: %d", rc );
                  goto error ;
               }
               if ( !dmsAccessAndFlagCompatiblity ( _context->mb()->_flag,
                                                    _accessType ) )
               {
                  PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                           _context->mb()->_flag ) ;
                  rc = SDB_DMS_INCOMPATIBLE_MODE ;
                  goto error ;
               }
               rc = _scanner->resumeScan( _recordXLock ? FALSE : TRUE ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Failed to resume ixscan, rc: %d", rc ) ;
                  goto error ;
               }
            }
         }

         if ( _curRecordPtr->isDeleting() )
         {
            if ( _recordXLock )
            {
               rc = _scanner->pauseScan( _recordXLock ? FALSE : TRUE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to pause ixscan, rc: %d", rc ) ;

               rc = _pSu->deleteRecord( _context, _curRID, 0, cb, NULL ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDWARNING, "Failed to delete the deleting record, "
                          "rc: %d", rc ) ;
               }

               rc = _scanner->resumeScan( _recordXLock ? FALSE : TRUE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to resume ixscan, rc: %d", rc ) ;

               _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                            _context->mbID(), &_curRID ) ;
               lockedRecord = FALSE ;
            }
            continue ;
         }
         SDB_ASSERT( !_curRecordPtr->isDeleted(), "record can't be deleted" ) ;

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
               BSONObj obj ( recordData.data() ) ;
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

         if ( _recordXLock )
         {
            _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                         _context->mbID(), &_curRID ) ;
            lockedRecord = FALSE ;
         }
      } // while

      rc = SDB_DMS_EOC ;
      {
         INT32 rcTmp = _scanner->pauseScan( _recordXLock ? FALSE : TRUE ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Pause scan failed, rc: %d", rcTmp ) ;
            rc = rcTmp ;
         }
      }
      goto error ;

   done:
      if ( SDB_OK == rc && ( _onceRestNum <= 0 || 0 == _maxRecords ) )
      {
         rc = _scanner->pauseScan( _recordXLock ? FALSE : TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Pause scan failed, rc: %d", rc ) ;
         }
      }

      return rc ;
   error:
      if ( lockedRecord && _recordXLock )
      {
         _pTransCB->transLockRelease( cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID ) ;
      }
      recordID.reset() ;
      recordDataPtr = 0 ;
      generator.setDataPtr( recordDataPtr ) ;
      _curRID._offset = DMS_INVALID_OFFSET ;
      goto done ;
   }

   void _dmsIXSecScanner::stop ()
   {
      if ( FALSE == _firstRun && _recordXLock &&
           DMS_INVALID_OFFSET != _curRID._offset )
      {
         _pTransCB->transLockRelease( _cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID ) ;
      }
      if ( DMS_INVALID_OFFSET != _curRID._offset )
      {
         INT32 rc = _scanner->pauseScan( _recordXLock ? FALSE : TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Pause scan failed, rc: %d", rc ) ;
         }
      }
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
                                 INT64 skipNum )
   :_dmsScanner( su, context, matchRuntime, accessType ),
    _secScanner( su, context, matchRuntime, scanner, accessType, maxRecords,
                 skipNum )
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
            rc = _context->mbLock( ( DMS_ACCESS_TYPE_UPDATE == _accessType ||
                                     DMS_ACCESS_TYPE_DELETE == _accessType ||
                                     DMS_ACCESS_TYPE_INSERT == _accessType ) ?
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
                                                     INT64 skipNum )
   {
      dmsExtScannerBase* scanner = NULL ;

      if ( OSS_BIT_TEST( DMS_MB_ATTR_CAPPED, context->mb()->_attributes ) )
      {
         scanner = SDB_OSS_NEW dmsCappedExtScanner( su, context,
                                                    matchRuntime,
                                                    curExtentID,
                                                    accessType,
                                                    maxRecords,
                                                    skipNum ) ;
      }
      else
      {
         scanner = SDB_OSS_NEW dmsExtScanner( su, context,
                                              matchRuntime,
                                              curExtentID,
                                              accessType,
                                              maxRecords,
                                              skipNum ) ;
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


