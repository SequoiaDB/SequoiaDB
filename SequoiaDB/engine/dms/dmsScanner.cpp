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
#include "dms.hpp"
#include "dmsOprHandler.hpp"
#include "dmsReadUnit.hpp"
#include "dmsStorageIndex.hpp"
#include "dmsStorageDataCommon.hpp"
#include "rtnTBScanner.hpp"
#include "rtnIXScanner.hpp"
#include "rtnDiskIXScanner.hpp"
#include "rtnMergeIXScanner.hpp"
#include "rtnScannerFactory.hpp"
#include "bpsPrefetch.hpp"
#include "dmsCompress.hpp"
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

   /*
      _dmsIndexRecordRW implement
    */
   class _dmsIndexRecordRW : public _dmsRecordRW
   {
      public:
         _dmsIndexRecordRW( const _dmsRecordRW &recordRW, const CHAR * ptr )
         :_dmsRecordRW( recordRW )
         {
            if ( ptr )
            {
               _isDirectMem = TRUE ;
               _ptr = ( const dmsRecord* )ptr ;
            }
         }
   } ;
   typedef _dmsIndexRecordRW dmsIndexRecordRW ;

   /*
      _dmsIndexCoverRecordBuilder define and implement
    */
   class _dmsIndexCoverRecordBuilder
   {
   public:
      _dmsIndexCoverRecordBuilder( CHAR *pBuf )
      {
         _pBuf    = pBuf ;
         _pCur    = _pBuf ;
         _ppPos   = NULL ;
         _hasDone = FALSE ;

         init() ;
      }

      _dmsIndexCoverRecordBuilder( CHAR **ppPos )
      {
         _pBuf    = *ppPos ;
         _pCur    = _pBuf ;
         _ppPos   = ppPos ;
         _hasDone = FALSE ;

         init() ;
      }

      ~_dmsIndexCoverRecordBuilder()
      {
         done() ;
      }

      BOOLEAN isEmpty() const
      {
         return len() <= 5 ? TRUE : FALSE ;
      }

      UINT32 len() const
      {
         return _pCur - _pBuf ;
      }

      const CHAR *done()
      {
         if ( !_hasDone )
         {
            _hasDone = TRUE ;
            *_pCur = (CHAR)EOO ;
            ++ _pCur ;
            /// set size
            *( (UINT32 *)_pBuf ) = _pCur - _pBuf ;
            /// set pos
            if ( _ppPos )
            {
               *_ppPos = _pCur ;
            }
         }
         return _pBuf ;
      }

      _dmsIndexCoverRecordBuilder *appendElement( BSONElement &ele )
      {
         ossMemcpy( _pCur, ele.rawdata(), ele.size() ) ;
         _pCur += ele.size() ;
         return this ;
      }

      _dmsIndexCoverRecordBuilder* appendAs( const BSONElement &e,
                                             const StringData &fieldName )
      {
         *_pCur = (CHAR)( e.type() ) ;
         _pCur += 1 ;

         INT32 len ;
         len = fieldName.size() ;
         ossMemcpy( _pCur, fieldName.data(), len ) ;
         _pCur += len ;
         *_pCur = 0 ;
         _pCur += 1 ;

         len = e.valuesize() ;
         ossMemcpy( _pCur, (void *)( e.value() ), len ) ;
         _pCur += len ;

         return this ;
      }

      CHAR **subobjStart( const StringData &fieldName )
      {
         *_pCur = (CHAR) Object ;
         _pCur += 1 ;

         const INT32 len = fieldName.size() ;
         ossMemcpy( _pCur, fieldName.data(), len ) ;
         _pCur += len ;
         *_pCur = 0 ;
         _pCur += 1 ;

         return &_pCur ;
      }

      void abortSubobj( const StringData &fieldName, _dmsIndexCoverRecordBuilder &sub )
      {
         _pCur -= 1 ;
         const INT32 len = fieldName.size() + 1 ;
         _pCur -= len ;
         _pCur -= sub.len() ;
      }

      static BOOLEAN buildObj( ixmIndexNode *node,
                               IXM_ELE_RAWDATA_ARRAY &value,
                               _dmsIndexCoverRecordBuilder &builder ) ;

   protected:
      void init()
      {
         *((UINT32*)_pBuf) = 0 ;
         _pCur = _pBuf + 4 ;
      }

   private:
      CHAR * _pBuf ;
      CHAR * _pCur ;
      CHAR ** _ppPos ;
      BOOLEAN _hasDone ;
   } ;

   typedef class _dmsIndexCoverRecordBuilder dmsIndexCoverRecordBuilder ;

   BOOLEAN _dmsIndexCoverRecordBuilder::buildObj( ixmIndexNode *node,
                                                  IXM_ELE_RAWDATA_ARRAY &value,
                                                  _dmsIndexCoverRecordBuilder &builder )
   {
      BOOLEAN finished = FALSE ;

      // node tree:      a
      //                 |
      //            b(1) c(2) d(EOO)
      // then builder obj is : a{b:1,c:2}

      IXM_INDEX_NODE_PTR_ARRAY &children = node->getChildren() ;

      try
      {
         for ( UINT32 i = 0; i < node->childrenSize(); i++ )
         {
            if( 0 == children[i]->childrenSize() )
            {
               UINT32 fieldIndex = children[i]->getFieldIndex() ;
               SDB_ASSERT( fieldIndex < value.size(), "Field index bigger than field size" ) ;

               BSONElement ele( value[ fieldIndex ] ) ;

               if( Undefined != ele.type() )
               {
                  builder.appendAs( ele, children[i]->getName() );
               }
               else if( children[i]->isEmbedded() )
               {
                  // if index fields is {"a.b":1,c:1},insert {a:10,c:10}
                  // key value is {"":{"Undefined":1},"c":10}
                  // dms value is {a:10,c:10}
                  // not the same so we should to read dms value again
                  goto done ;
               }
            }
            else
            {
               _dmsIndexCoverRecordBuilder sub(
                           builder.subobjStart( children[i]->getName() ) ) ;
               if( FALSE == buildObj( children[i], value, sub ) )
               {
                  goto done ;
               }
               sub.done() ;
               if( sub.isEmpty() )
               {
                  builder.abortSubobj( children[i]->getName(), sub ) ;
               }
            }
         }
         finished = TRUE ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "Failed to build index value object, "
                 "occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      return finished ;

   error:
      goto done ;
   }

   /*
      _dmsScannerContext implement
    */
   _dmsScannerContext::_dmsScannerContext( dmsSecScanner *scanner )
   : _hasPaused( FALSE ),
     _scanner( scanner )
   {
   }

   _dmsScannerContext::~_dmsScannerContext ()
   {
      _hasPaused = FALSE ;
      _scanner = NULL ;
   }

   INT32 _dmsScannerContext::pause()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isHolding = FALSE ;
      dpsTransRetInfo dpsTxResInfo ;
      dpsTransCB *transCB = sdbGetTransCB() ;

      isHolding = transCB->transIsHolding( _scanner->getEDUCB(),
                                           _scanner->getDataSU()->logicalID(),
                                           _scanner->getMBContext()->mbID(),
                                           &_scanner->getAdvancedRecordID() ) ;

      if ( isHolding )
      {
         _hasPaused = TRUE ;
         // don't need to release transaction locks here, since we need them holding
         return  _scanner->getScanner()->pauseScan() ;
      }

      return rc ;
   }

   INT32 _dmsScannerContext::resume()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isCursorSame = FALSE ;

      if ( !_hasPaused )
      {
         goto done ;
      }

      _hasPaused = FALSE ;
      rc  = _scanner->getScanner()->resumeScan( isCursorSame ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resume scan, rc: %d", rc ) ;

      SDB_ASSERT( TRUE == isCursorSame, "Must be same" ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _dmsScanner implement
   */
   _dmsScanner::_dmsScanner( dmsStorageDataCommon *su,
                             dmsMBContext *context,
                             mthMatchRuntime *matchRuntime,
                             pmdEDUCB *cb,
                             DMS_ACCESS_TYPE accessType,
                             INT64 maxRecords,
                             INT64 skipNum,
                             INT32 flags,
                             IDmsOprHandler *opHandler )
   {
      SDB_ASSERT( su, "storage data can't be NULL" ) ;
      SDB_ASSERT( context, "context can't be NULL" ) ;
      _pSu = su ;
      _context = context ;
      _matchRuntime = matchRuntime ;
      _accessType = accessType ;
      _mbLockType = SHARED ;

      if ( DMS_IS_WRITE_OPR( _accessType ) )
      {
         if ( cb->getTransExecutor()->useTransLock() )
         {
            _mbLockType = su->getWriteLockType() ;
         }
         else
         {
            _mbLockType = EXCLUSIVE ;
         }
      }

      _maxRecords = maxRecords ;
      _skipNum = skipNum ;
      _flags = flags ;

      _opHandler = opHandler ;
   }

   _dmsScanner::~_dmsScanner()
   {
      _context    = NULL ;
      _pSu        = NULL ;
   }

   void _dmsScanner::_saveAdvancedRecrodID( const dmsRecordID &recordID,
                                            INT32 rc )
   {
      if ( SDB_OK == rc )
      {
         _advancedRecordID = recordID ;
      }
      else
      {
         _advancedRecordID.reset() ;
      }
   }

   void _dmsScanner::_checkMaxRecordsNum( _mthRecordGenerator &generator )
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

   /*
      _dmsScannerLockHandler implement
    */
   _dmsScannerLockHandler::_dmsScannerLockHandler( IDmsOprHandler *opHandler,
                                                   INT32 flags )
   : _isInited( FALSE ),
     _pTransCB( pmdGetKRCB()->getTransCB() ),
     _transIsolation( TRANS_ISOLATION_RU ),
     _waitLock( FALSE ),
     _useRollbackSegment( TRUE ),
     _needEscalation( FALSE ),
     _hasLockedRecord( FALSE ),
     _recordLock( DPS_TRANSLOCK_MAX ),
     _selectLockMode( DPS_TRANSLOCK_MAX ),
     _lockOpMode( DPS_TRANSLOCK_OP_MODE_ACQUIRE ),
     _needUnLock( FALSE ),
     _CSCLLockHeld( FALSE ),
     _callback( opHandler )
   {
      // lock for update has higher priority
      if ( OSS_BIT_TEST( flags, FLG_QUERY_FOR_UPDATE ) )
      {
         _selectLockMode = DPS_TRANSLOCK_U ;
      }
      else if ( OSS_BIT_TEST( flags, FLG_QUERY_FOR_SHARE ) )
      {
         _selectLockMode = DPS_TRANSLOCK_S ;
      }
   }

   _dmsScannerLockHandler::~_dmsScannerLockHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCANLOCKHANDLER__ACQUIRECSCLLOCK, "_dmsScannerLockHandler::_acquireCSCLLock" )
   INT32 _dmsScannerLockHandler::_acquireCSCLLock( _dmsStorageDataCommon *su,
                                                   _dmsMBContext *mbContext,
                                                   pmdEDUCB *cb,
                                                   IContext *transContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSCANLOCKHANDLER__ACQUIRECSCLLOCK ) ;

      if ( !_CSCLLockHeld && DPS_TRANSLOCK_MAX != _recordLock )
      {
         dpsTransRetInfo   lockConflict ;
         if ( DPS_TRANSLOCK_IS == dpsIntentLockMode( _recordLock ) )
         {
            rc = _pTransCB->transLockGetIS( cb,
                                            su->logicalID(),
                                            mbContext->mbID(),
                                            transContext,
                                            &lockConflict ) ;
         }
         else if ( DPS_TRANSLOCK_IX == dpsIntentLockMode( _recordLock ) )
         {
            rc = _pTransCB->transLockGetIX( cb,
                                            su->logicalID(),
                                            mbContext->mbID(),
                                            transContext,
                                            &lockConflict ) ;
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
                      "Failed to get CS/CL lock, rc: %d" OSS_NEWLINE
                      "Conflict ( representative ):" OSS_NEWLINE
                      "   EDUID:  %llu" OSS_NEWLINE
                      "   TID:    %u" OSS_NEWLINE
                      "   LockId: %s" OSS_NEWLINE
                      "   Mode:   %s" OSS_NEWLINE,
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
      PD_TRACE_EXITRC( SDB__DMSSCANLOCKHANDLER__ACQUIRECSCLLOCK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCANLOCKHANDLER__RELEASECSCLLOCK, "_dmsScannerLockHandler::_releaseCSCLLock" )
   void _dmsScannerLockHandler::_releaseCSCLLock( _dmsStorageDataCommon *su,
                                                  _dmsMBContext *mbContext,
                                                  pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__DMSSCANLOCKHANDLER__RELEASECSCLLOCK ) ;

      if ( _CSCLLockHeld )
      {
         _pTransCB->transLockRelease( cb,
                                      su->logicalID(),
                                      mbContext->mbID() ) ;
         _CSCLLockHeld = FALSE ;
      }

      PD_TRACE_EXIT( SDB__DMSSCANLOCKHANDLER__RELEASECSCLLOCK ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCANLOCKHANDLER__INITLOCKINFO_CB, "_dmsScannerLockHandler::_initLockInfo" )
   void _dmsScannerLockHandler::_initLockInfo( _dmsStorageDataCommon *su,
                                               _dmsMBContext *mbContext,
                                               DMS_ACCESS_TYPE accessType,
                                               pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__DMSSCANLOCKHANDLER__INITLOCKINFO_CB ) ;

      if ( !_isInited )
      {
         dpsTransExecutor *pExe = cb->getTransExecutor() ;

         _transIsolation = pExe->getTransIsolation() ;
         _waitLock = pExe->isTransWaitLock() ;
         _useRollbackSegment = pExe->useRollbackSegment() ;

         _lockOpMode = DPS_TRANSLOCK_OP_MODE_ACQUIRE ;

         /// When not support trans
         if ( !su->isTransLockRequired( mbContext ) )
         {
            _recordLock = DPS_TRANSLOCK_MAX ;
         }
         /// When not in transaction
         else if ( ( DPS_INVALID_TRANS_ID == cb->getTransID() ) ||
                   ( !su->isTransSupport( mbContext ) ) )
         {
            /// When not use trans lock
            if ( !pExe->useTransLock() )
            {
               _recordLock = DPS_TRANSLOCK_MAX ;
            }
            /// Write operation should release lock right now
            else if ( DMS_IS_WRITE_OPR( accessType ) )
            {
               _recordLock = DPS_TRANSLOCK_X ;
               _needUnLock = TRUE ;
               _useRollbackSegment = FALSE ;
            }
            /// Read is always no lock
            else
            {
               _recordLock = DPS_TRANSLOCK_MAX ;
            }
         }
         /// In transaction
         else
         {
            if ( cb->isInTransRollback() )
            {
               _recordLock = DPS_TRANSLOCK_MAX ;
            }
            else if ( !pExe->useTransLock() )
            {
               _recordLock = DPS_TRANSLOCK_MAX ;
            }
            else if ( DMS_IS_WRITE_OPR( accessType ) )
            {
               _recordLock = DPS_TRANSLOCK_X ;
               _needUnLock = FALSE ;
               _needEscalation = TRUE ;
            }
            else if ( TRANS_ISOLATION_RU == _transIsolation &&
                      DPS_TRANSLOCK_MAX == _selectLockMode )
            {
               _recordLock = DPS_TRANSLOCK_MAX ;
            }
            else
            {
               _recordLock =
                     DPS_TRANSLOCK_MAX != _selectLockMode ?
                                                _selectLockMode :
                                                DPS_TRANSLOCK_S ;
               if ( TRANS_ISOLATION_RS == _transIsolation ||
                    DPS_TRANSLOCK_MAX != _selectLockMode )
               {
                  _needUnLock = FALSE ;
                  _waitLock = TRUE ;
                  _needEscalation = TRUE ;
               }
               else
               {
                  _needUnLock = TRUE ;
               }
            }
         }

         _isInited = TRUE ;
      }

      PD_TRACE_EXIT( SDB__DMSSCANLOCKHANDLER__INITLOCKINFO_CB ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCANLOCKHANDLER__INITLOCKINFO_INFO, "_dmsScannerLockHandler::_initLockInfo" )
   void _dmsScannerLockHandler::_initLockInfo( INT32 isolation,
                                               DPS_TRANSLOCK_TYPE lockType,
                                               DPS_TRANSLOCK_OP_MODE_TYPE lockOpMode )
   {
      PD_TRACE_ENTRY( SDB__DMSSCANLOCKHANDLER__INITLOCKINFO_INFO ) ;

      if ( !_isInited )
      {
         _transIsolation = isolation ;
         _recordLock = lockType ;
         _selectLockMode = lockType ;
         _lockOpMode = lockOpMode ;

         _useRollbackSegment = FALSE ;
         _waitLock = TRUE ;
         _needUnLock = FALSE ;
         _isInited = TRUE ;
      }

      PD_TRACE_EXIT( SDB__DMSSCANLOCKHANDLER__INITLOCKINFO_INFO ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCANLOCKHANDLER__CHECKTRANSLOCK, "_dmsScannerLockHandler::_checkTransLock" )
   INT32 _dmsScannerLockHandler::_checkTransLock( _dmsStorageDataCommon *su,
                                                  _dmsMBContext *mbContext,
                                                  const dmsRecordID &curRID,
                                                  pmdEDUCB *cb,
                                                  dmsScanTransContext *transContext,
                                                  dmsRecordRW &recordRW,
                                                  dmsRecordID &waitUnlockRID,
                                                  BOOLEAN &skipRecord )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSCANLOCKHANDLER__CHECKTRANSLOCK ) ;

      BOOLEAN ignoredLock = FALSE ;
      dpsTransRetInfo lockConflict ;

      if ( DPS_TRANSLOCK_MAX == _recordLock )
      {
         goto done ;
      }

      /// already locked, but not the same, should release lock first
      if ( _hasLockedRecord &&
           waitUnlockRID.isValid() &&
           curRID != waitUnlockRID )
      {
         _pTransCB->transLockRelease( cb,
                                      su->logicalID(),
                                      mbContext->mbID(),
                                      &waitUnlockRID,
                                      &_callback ) ;
         waitUnlockRID.reset() ;
         _hasLockedRecord = FALSE ;
      }

      // attach the recordRW in callback
      _callback.attachRecordRW( &recordRW ) ;
      _callback.clearStatus() ;

      if ( DPS_TRANSLOCK_X == _recordLock )
      {
         // exclusive lock has to always wait on the lock
         rc = _pTransCB->transLockGetX( cb,
                                        su->logicalID(),
                                        mbContext->mbID(),
                                        &curRID,
                                        transContext,
                                        &lockConflict,
                                        &_callback ) ;
      }
      else if ( DPS_TRANSLOCK_U == _recordLock )
      {
         rc = _pTransCB->transLockGetU( cb, su->logicalID(),
                                        mbContext->mbID(),
                                        &curRID,
                                        transContext,
                                        &lockConflict,
                                        &_callback ) ;
      }
      // DPS_TRANSLOCK_S
      else
      {
         if ( !_waitLock )
         {
            // for new RC logic, we should first test on S lock instead
            // of directly wait on the record lock. Under the cover,
            // the lock call back function would try to use the old copy
            // (previous committed version) if exist
            rc = _pTransCB->transLockTestSPreempt( cb,
                                                   su->logicalID(),
                                                   mbContext->mbID(),
                                                   &curRID,
                                                   &lockConflict,
                                                   &_callback,
                                                   !_CSCLLockHeld ) ;
            ignoredLock = TRUE ;
            if ( _callback.isSkipRecord() )
            {
               _onRecordSkipped( curRID, transContext ) ;
               rc = SDB_OK ;
               skipRecord = TRUE ;
               goto done ;
            }
            if ( _callback.isUseOldVersion() )
            {
               rc = SDB_OK ;
            }
         }

         /// wait lock
         if ( _waitLock || rc )
         {
            // test S lock failed and the record is not in old version
            // container nor in RBS. most likely the one hold / wait X
            // hasn't finish updating the record.
            // NOTE: RS and lock for share requires lock escalation
            rc = _pTransCB->transLockGetS( cb,
                                           su->logicalID(),
                                           mbContext->mbID(),
                                           &curRID,
                                           transContext,
                                           &lockConflict,
                                           &_callback,
                                           _needEscalation ) ;
            if ( SDB_OK == rc )
            {
               ignoredLock = FALSE ;
            }
         }
      }

      if ( rc )
      {
         PD_LOG( PDERROR,
                  "Failed to get record lock, rc: %d" OSS_NEWLINE
                  "Request Mode:   %s" OSS_NEWLINE
                  "Conflict ( representative ):" OSS_NEWLINE
                  "   EDUID:  %llu" OSS_NEWLINE
                  "   TID:    %u" OSS_NEWLINE
                  "   LockId: %s" OSS_NEWLINE
                  "   Mode:   %s" OSS_NEWLINE,
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

      _onRecordLocked( curRID, transContext, skipRecord ) ;

   done:
      _callback.detachRecordRW() ;
      PD_TRACE_EXITRC( SDB__DMSSCANLOCKHANDLER__CHECKTRANSLOCK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void _dmsScannerLockHandler::_releaseTransLock( dmsStorageDataCommon *su,
                                                   dmsMBContext *mbContext,
                                                   const dmsRecordID &curRID,
                                                   pmdEDUCB *cb )
   {
      if ( _hasLockedRecord && curRID.isValid() )
      {
         if ( _needUnLock )
         {
            // last run have record lock held, but not trans, need to release
            // record lock
            _pTransCB->transLockRelease( cb, su->logicalID(),
                                         mbContext->mbID(), &curRID,
                                         &_callback ) ;
            _hasLockedRecord = FALSE ;
         }
         else if ( NULL != cb &&
                   cb->getTransExecutor()->useTransLock() &&
                   _callback.getTransRecordInfo()->_transInsertDeleted )
         {
            SDB_ASSERT( !cb->isInTransRollback(), "should not be deleted by "
                        "table scan during trans rollback" ) ;
            // if the record is deleted in the same transaction, we can
            // release the lock
            // NOTE: we need to keep the IX locks on CS and CL
            _pTransCB->transLockRelease( cb, su->logicalID(),
                                         mbContext->mbID(), &curRID,
                                         &_callback, TRUE, FALSE ) ;

            _hasLockedRecord = FALSE ;
         }
      }
   }

   void _dmsScannerLockHandler::_releaseAllLocks( dmsStorageDataCommon *su,
                                                  dmsMBContext *mbContext,
                                                  const dmsRecordID &curRID,
                                                  pmdEDUCB *cb )
   {
      _releaseTransLock( su, mbContext, curRID, cb ) ;
      _releaseCSCLLock( su, mbContext, cb ) ;
   }

   /*
      _dmsSecScanner implement
    */
   _dmsSecScanner::_dmsSecScanner( dmsStorageDataCommon *su,
                                   dmsMBContext *context,
                                   rtnScanner *scanner,
                                   mthMatchRuntime *matchRuntime,
                                   DMS_ACCESS_TYPE accessType,
                                   INT64 maxRecords,
                                   INT64 skipNum,
                                   INT32 flags,
                                   IDmsOprHandler *handler )
   : _dmsScanner( su, context, matchRuntime, scanner->getEDUCB(), accessType, maxRecords, skipNum, flags, handler ),
     _dmsScannerLockHandler( handler, flags ),
     _scanner( scanner ),
     _transContext( context, scanner, accessType ),
     _scannerContext( this ),
     _isCountOnly( FALSE ),
     _firstRun( TRUE ),
     _onceRestNum( pmdGetOptionCB()->indexScanStep() ),
     _cb( NULL )
   {
   }

   _dmsSecScanner::~_dmsSecScanner()
   {
      _releaseAllLocks( _pSu, _context, _curRID, _cb ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSECSCAN_ADVANCE, "_dmsSecScanner::advance" )
   INT32 _dmsSecScanner::advance( dmsRecordID &recordID,
                                  _mthRecordGenerator &generator,
                                  pmdEDUCB *cb,
                                  _mthMatchTreeContext *mthContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSECSCAN_ADVANCE ) ;

      if ( _firstRun )
      {
         rc = _firstInit( cb ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to call first init, rc: %d", rc ) ;

         // unset first run
         _firstRun = FALSE ;
      }
      else if ( _curRID.isValid() )
      {
         _releaseTransLock( _pSu, _context, _curRID, cb ) ;
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
      _saveAdvancedRecrodID( recordID, rc ) ;
      PD_TRACE_EXITRC( SDB__DMSSECSCAN_ADVANCE, rc ) ;
      return rc ;

   error:
      recordID.reset() ;
      goto done ;
   }

   void _dmsSecScanner::stop()
   {
      if ( _curRID.isValid() )
      {
         INT32 rc = _scanner->pauseScan() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to pause scanner, rc: %d", rc ) ;
         }
      }
      _releaseAllLocks( _pSu, _context, _curRID, _cb ) ;
      _curRID.reset() ;
   }

   void _dmsSecScanner::pause()
   {
      if ( _curRID.isValid() )
      {
         INT32 rc = _scanner->pauseScan() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to pause scanner, rc: %d", rc ) ;
         }
      }
      _releaseAllLocks( _pSu, _context, _curRID, _cb ) ;
      _context->pause() ;
      _firstRun = TRUE ;
   }

   dmsTransLockCallback* _dmsSecScanner::callbackHandler()
   {
      return &_callback ;
   }

   const dmsTransRecordInfo* _dmsSecScanner::recordInfo() const
   {
      return _callback.getTransRecordInfo() ;
   }

   BOOLEAN _dmsSecScanner::isHitEnd() const
   {
      return _scanner ? _scanner->isEOF() : TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSECSCAN__FIRSTINIT, "_dmsSecScanner::_firstInit" )
   INT32 _dmsSecScanner::_firstInit( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSECSCAN__FIRSTINIT ) ;

      _cb = cb ;

      _initLockInfo( _pSu, _context, _accessType, cb ) ;

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

      PD_CHECK( dmsAccessAndFlagCompatiblity( _context->mb()->_flag,
                                              _accessType ),
                SDB_DMS_INCOMPATIBLE_MODE, error, PDERROR,
                "Incompatible collection mode: %d", _context->mb()->_flag ) ;

      // set callback info
      _callback.setBaseInfo( _pTransCB, cb ) ;
      _callback.setIDInfo( _pSu->CSID(), _context->mbID(),
                           _pSu->logicalID(),
                           _context->clLID() ) ;

      // As a performance improvement, we are going to acquire the CS and
      // CL lock right in the beginning to avoid extra performance overhead
      // to acquire these locks when acquiring record lock in each step
      // We release and require the lock during pauseScan/resumeScan
      rc = _acquireCSCLLock( _pSu, _context, cb, &_transContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to acquired collection space and "
                   "collection locks, rc: %d", rc ) ;

      rc = _onFirstInit( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call first init, rc: %d", rc ) ;

      _callback.setScanner( _scanner ) ;

      _onceRestNum = _getOnceRestNum() ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSECSCAN__FIRSTINIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSECSCAN__FETCHNEXT, "_dmsSecScanner::_fetchNext" )
   INT32 _dmsSecScanner::_fetchNext( dmsRecordID &recordID,
                                     _mthRecordGenerator &generator,
                                     _pmdEDUCB *cb,
                                     _mthMatchTreeContext *mthContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSECSCAN__FETCHNEXT ) ;

      BOOLEAN result = TRUE ;
      ossValuePtr recordDataPtr ;
      dmsRecordID lastRID ;

      while ( ( !isHitEnd() ) &&
              ( _onceRestNum -- > 0 ) &&
              ( 0 != _maxRecords ) )
      {
         dmsRecordID nextRID ;
         dmsRecordRW recordRW ;
         rc = _advanceScanner( cb, nextRID ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               PD_LOG( PDERROR, "Failed to advance cursor, rc: %d", rc ) ;
            }
            goto error ;
         }

         if ( DPS_TRANSLOCK_MAX != _recordLock )
         {
            BOOLEAN skipRecord = FALSE ;
            _transContext.reset() ;
            rc = _checkTransLock( _pSu, _context, nextRID, cb, &_transContext,
                                  recordRW, lastRID, skipRecord ) ;
            lastRID.reset() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check transaction lock, rc: %d", rc ) ;

            if ( skipRecord )
            {
               lastRID = nextRID ;
               continue ;
            }
         }
         _curRID = nextRID ;
         lastRID = nextRID ;

         if ( !recordRW.isDirectMem() )
         {
            BOOLEAN isSnapshotSame = FALSE ;
            rc = _checkSnapshotID( isSnapshotSame ) ;
            if ( SDB_OK != rc )
            {
               if ( SDB_DMS_EOC != rc )
               {
                  PD_LOG( PDERROR, "Failed to check snapshot ID, rc: %d", rc ) ;
               }
               goto error ;
            }
            if ( !isSnapshotSame )
            {
               continue ;
            }
         }

         if ( !_matchRuntime && _skipNum > 0 )
         {
            if ( _skipNum > 0 )
            {
               --_skipNum ;
               continue ;
            }
            else if ( _isCountOnly )
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
         else
         {
            recordID = _curRID ;

            if ( !recordRW.isDirectMem() )
            {
               rc = _getCurrentRecord( _recordData ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get record data, rc: %d", rc ) ;
            }
            else
            {
               const dmsRecord *record = recordRW.readPtr( 0 ) ;
               _recordData.setData( record->getData(), record->getDataLength() ) ;
            }

            recordDataPtr = ( ossValuePtr )_recordData.data() ;
            generator.setDataPtr( recordDataPtr ) ;

            // math
            if ( _matchRuntime && _matchRuntime->getMatchTree() )
            {
               result = TRUE ;
               try
               {
                  _mthMatchTree *matcher = _matchRuntime->getMatchTree() ;
                  rtnParamList *parameters = _matchRuntime->getParametersPointer() ;
                  BSONObj obj( _recordData.data() ) ;
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
                        goto done ;
                     }
                  }
               }
               catch ( exception &e )
               {
                  PD_LOG( PDERROR, "Failed to create BSON object, occur exception: %s",
                          e.what() ) ;
                  rc = ossException2RC( &e ) ;
                  goto error ;
               }
            }
            else
            {
               try
               {
                  BSONObj obj( _recordData.data() ) ;
                  rc = generator.resetValue( obj, mthContext ) ;
                  PD_RC_CHECK( rc, PDERROR, "resetValue failed:rc=%d", rc ) ;
               }
               catch ( exception &e )
               {
                  PD_LOG( PDERROR, "Failed to create BSON object, occur exception: %s",
                          e.what() ) ;
                  rc = ossException2RC( &e ) ;
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
                  goto done ;
               }
            }
         }
      }

      if ( DPS_TRANSLOCK_MAX != _recordLock &&
           _hasLockedRecord &&
           lastRID.isValid() )
      {
         _pTransCB->transLockRelease( cb,
                                      _pSu->logicalID(),
                                      _context->mbID(),
                                      &lastRID,
                                      &_callback ) ;
         lastRID.reset() ;
         _hasLockedRecord = FALSE ;
      }

      // pause scanner on section EOC
      rc = _scanner->pauseScan() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to pause scan, rc: %d", rc ) ; ;
      rc = SDB_DMS_EOC ;
      goto error ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSECSCAN__FETCHNEXT, rc ) ;
      return rc ;

   error:
      if ( DPS_TRANSLOCK_MAX != _recordLock &&
           _hasLockedRecord &&
           lastRID.isValid() )
      {
         _pTransCB->transLockRelease( cb,
                                      _pSu->logicalID(),
                                      _context->mbID(),
                                      &lastRID,
                                      &_callback ) ;
         lastRID.reset() ;
         _hasLockedRecord = FALSE ;
      }

      _releaseAllLocks( _pSu, _context, _curRID, _cb ) ;

      recordID.reset() ;
      recordDataPtr = 0 ;
      generator.setDataPtr( recordDataPtr ) ;

      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSECSCAN__ONRECSKIPPED, "_dmsSecScanner::_onRecordSkipped" )
   void _dmsSecScanner::_onRecordSkipped( const dmsRecordID &curRID,
                                          dmsScanTransContext *transContext )
   {
      PD_TRACE_ENTRY( SDB__DMSSECSCAN__ONRECSKIPPED ) ;

      _scanner->removeDuplicatRID( curRID ) ;

      PD_TRACE_EXIT( SDB__DMSSECSCAN__ONRECSKIPPED ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSECSCAN__ONRECLOCKED, "_dmsSecScanner::_onRecordLocked" )
   void _dmsSecScanner::_onRecordLocked( const dmsRecordID &curRID,
                                         dmsScanTransContext *transContext,
                                         BOOLEAN &skipRecord )
   {
      PD_TRACE_ENTRY( SDB__DMSSECSCAN__ONRECLOCKED ) ;

      if ( !transContext->isCursorSame() || _callback.isSkipRecord() )
      {
         /// remove the duplicate key
         _scanner->removeDuplicatRID( curRID ) ;

#ifdef _DEBUG
         PD_LOG( PDDEBUG, "Cursor changed while waiting for lock, "
                 "rid(%d, %d), isCursorSame(%d), _onceRestNum(%d), "
                 "isSkipRecord(%d)",
                 curRID._extent, curRID._offset,
                 transContext->isCursorSame(), _onceRestNum,
                 _callback.isSkipRecord()) ;
#endif
         // When cursor changed, we may need to go back to previous
         // key to retry, don't count as a step. Also avoid potential
         // pause here if step becomes 0, in which case we may unexpectly
         // lose previously savedObj and savedRID and cause skip record.
         if ( !transContext->isCursorSame() )
         {
            ++ _onceRestNum ;
         }

         skipRecord = TRUE ;
      }

      PD_TRACE_EXIT( SDB__DMSSECSCAN__ONRECLOCKED ) ;
   }

   /*
      _dmsDataScanner implement
    */
   _dmsDataScanner::_dmsDataScanner( dmsStorageDataCommon *su,
                                     dmsMBContext *context,
                                     _rtnTBScanner *scanner,
                                     mthMatchRuntime *matchRuntime,
                                     DMS_ACCESS_TYPE accessType,
                                     INT64 maxRecords,
                                     INT64 skipNum,
                                     INT32 flags,
                                     IDmsOprHandler *opHandler )
   : _dmsSecScanner( su,
                     context,
                     scanner,
                     matchRuntime,
                     accessType,
                     maxRecords,
                     skipNum,
                     flags,
                     opHandler ),
     _scanner( scanner )
   {
      SDB_ASSERT( NULL != scanner, "scanner should not be NULL" ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATASCAN__ONFIRSTINIT, "_dmsDataScanner::_onFirstInit" )
   INT32 _dmsDataScanner::_onFirstInit( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__DMSDATASCAN__ONFIRSTINIT ) ;

      BOOLEAN isCursorSame = FALSE ;

      PD_CHECK( _scanner, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to init scanner, scanner is invalid" ) ;

      if ( DPS_TRANSLOCK_MAX == _recordLock ||
           cb->getTransExecutor()->isLockEscalated( LOCKMGR_TRANS_LOCK ) )
      {
         _scanner->disableByType( SCANNER_TYPE_MEM_TREE ) ;
      }

      rc = _scanner->resumeScan( isCursorSame ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resum scanner, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSDATASCAN__ONFIRSTINIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATASCAN__ONADVANCESCANNER, "_dmsDataScanner::_advanceScanner" )
   INT32 _dmsDataScanner::_advanceScanner( pmdEDUCB *cb, dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATASCAN__ONADVANCESCANNER ) ;

      rc = _scanner->advance( rid ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Failed to advance scanner, rc: %d", rc ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSDATASCAN__ONADVANCESCANNER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATASCAN__CHECKSNAPSHOTID, "_dmsDataScanner::_checkSnapshotID" )
   INT32 _dmsDataScanner::_checkSnapshotID( BOOLEAN &isSnapshotSame )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATASCAN__CHECKSNAPSHOTID ) ;

      rc = _scanner->checkSnapshotID( isSnapshotSame ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check snapshot ID, rc: %d", rc ) ;

      if ( !isSnapshotSame )
      {
         rc = _scanner->pauseScan() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to pause scan, rc: %d", rc ) ; ;

         rc = _scanner->resumeScan( isSnapshotSame ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               PD_LOG( PDERROR, "Failed to resume scanner, rc: %d", rc ) ;
            }
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSDATASCAN__CHECKSNAPSHOTID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATASCAN__GETCURRID, "_dmsDataScanner::_getCurrentRID" )
   INT32 _dmsDataScanner::_getCurrentRID( dmsRecordID &nextRID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATASCAN__GETCURRID ) ;

      nextRID = _curRID ;

      PD_TRACE_EXITRC( SDB__DMSDATASCAN__GETCURRID, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATASCAN__GETCURREC, "_dmsDataScanner::_getCurrentRecord" )
   INT32 _dmsDataScanner::_getCurrentRecord( dmsRecordData &recordData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATASCAN__GETCURREC ) ;

      rc = _scanner->getCurrentRecord( recordData ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSDATASCAN__GETCURREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   UINT64 _dmsDataScanner::_getOnceRestNum() const
   {
      return (UINT64)( pmdGetOptionCB()->indexScanStep() * 10 ) ;
   }

   /*
      _dmsIndexScanner implement
    */
   _dmsIndexScanner::_dmsIndexScanner( dmsStorageDataCommon *su,
                                       dmsMBContext *context,
                                       rtnIXScanner *scanner,
                                       mthMatchRuntime *matchRuntime,
                                       DMS_ACCESS_TYPE accessType,
                                       INT64 maxRecords,
                                       INT64 skipNum,
                                       INT32 flags,
                                       IDmsOprHandler *opHandler )
   : _dmsSecScanner( su,
                     context,
                     scanner,
                     matchRuntime,
                     accessType,
                     maxRecords,
                     skipNum,
                     flags,
                     opHandler ),
     _scanner( scanner )
   {
      SDB_ASSERT( NULL != scanner, "scanner should not be NULL" ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIDXSCAN__ONFIRSTINIT, "_dmsIndexScanner::_onFirstInit" )
   INT32 _dmsIndexScanner::_onFirstInit( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__DMSIDXSCAN__ONFIRSTINIT ) ;

      BOOLEAN isCursorSame = FALSE ;

      PD_CHECK( _scanner, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to init scanner, scanner is invalid" ) ;

      _scanner->setReadonly( isReadOnly() ) ;
      if ( DPS_TRANSLOCK_MAX == _recordLock ||
           cb->getTransExecutor()->isLockEscalated( LOCKMGR_TRANS_LOCK ) )
      {
         _scanner->disableByType( SCANNER_TYPE_MEM_TREE ) ;
      }

      rc = _scanner->resumeScan( isCursorSame ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resum index scanner, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSIDXSCAN__ONFIRSTINIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIDXSCAN__ONADVANCESCANNER, "_dmsIndexScanner::_advanceScanner" )
   INT32 _dmsIndexScanner::_advanceScanner( pmdEDUCB *cb, dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSIDXSCAN__ONADVANCESCANNER ) ;

      rc = _scanner->advance( rid ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_IXM_EOC != rc )
         {
            PD_LOG( PDERROR, "Failed to advance scanner, rc: %d", rc ) ;
         }
         else
         {
            rc = SDB_DMS_EOC ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSIDXSCAN__ONADVANCESCANNER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIDXSCAN__CHECKSNAPSHOTID, "_dmsIndexScanner::_checkSnapshotID" )
   INT32 _dmsIndexScanner::_checkSnapshotID( BOOLEAN &isSnapshotSame )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSIDXSCAN__CHECKSNAPSHOTID ) ;

      rc = _scanner->checkSnapshotID( isSnapshotSame ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check snapshot ID, rc: %d", rc ) ;

      if ( !isSnapshotSame )
      {
         rc = _scanner->pauseScan() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to pause scan, rc: %d", rc ) ; ;

         rc = _scanner->resumeScan( isSnapshotSame ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_IXM_EOC != rc )
            {
               PD_LOG( PDERROR, "Failed to resume scanner, rc: %d", rc ) ;
            }
            else
            {
               rc = SDB_DMS_EOC ;
            }
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSIDXSCAN__CHECKSNAPSHOTID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIDXSCAN__GETCURRID, "_dmsIndexScanner::_getCurrentRID" )
   INT32 _dmsIndexScanner::_getCurrentRID( dmsRecordID &nextRID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSIDXSCAN__GETCURRID ) ;

      nextRID = _curRID ;

      PD_TRACE_EXITRC( SDB__DMSIDXSCAN__GETCURRID, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIDXSCAN__GETCURREC, "_dmsIndexScanner::_getCurrentRecord" )
   INT32 _dmsIndexScanner::_getCurrentRecord( dmsRecordData &recordData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSIDXSCAN__GETCURREC ) ;

      if( ( _scanner->isIndexCover() ) &&
          ( DMS_IS_READ_OPR( _accessType ) ) )
      {
         const CHAR *record = _buildIndexRecord() ;
         if ( NULL != record )
         {
            dmsRecordRW recordRW ;

            recordRW = dmsIndexRecordRW( recordRW, record ) ;
            const dmsRecord *record = recordRW.readPtr( 0 ) ;
            recordData.setData( record->getData(), record->getDataLength() ) ;
            goto done ;
         }
      }

      {
         dmsReadUnitScope readUnit( _scanner->getSession(), _cb ) ;
         rc = _pSu->extractData( _context, _curRID, _cb, recordData, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get record, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSIDXSCAN__GETCURREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   UINT64 _dmsIndexScanner::_getOnceRestNum() const
   {
      return (UINT64)( pmdGetOptionCB()->indexScanStep() ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIXSCAN__BUILDIDINDEXRECORD, "_dmsIndexScanner::_buildIndexRecord" )
   const CHAR *_dmsIndexScanner::_buildIndexRecord()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSIXSCAN__BUILDIDINDEXRECORD ) ;

      dmsRecord *pNewRecord = NULL ;
      ixmIndexCover &index = _scanner->getIndex() ;
      const BSONObj *keyValue = _scanner->getCurKeyObj() ;
      CHAR *recordPtr = NULL ;
      BSONObjIterator iter( *keyValue ) ;
      IXM_ELE_RAWDATA_ARRAY& container = index.getContainer() ;
      UINT32 extraSize = 0 ;
      UINT32 evalBufSize = 0 ;

      //1. pre caculte buf size
      rc = index.getExtraSize( extraSize ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get index value extra size, rc: %d", rc ) ;

      evalBufSize = DMS_RECORD_METADATA_SZ + extraSize + keyValue->objsize() ;

      rc = index.ensureBuff( evalBufSize, recordPtr ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get index buffer [%u], rc: %d",
                   evalBufSize, rc ) ;

      try
      {
         //2. parse keyValue element to vector
         //   index node tree will find element by vector index
         rc = index.reInitContainer() ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to reserve container space, rc: %d", rc ) ;
         while( iter.more() )
         {
            rc = container.append( iter.next().rawdata() ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to append index field value, rc: %d", rc ) ;
         }

         //3. reset header
         ossMemset( recordPtr, 0, DMS_RECORD_METADATA_SZ ) ;
         //4. build body(BSONObj)
         _dmsIndexCoverRecordBuilder builder( recordPtr + DMS_RECORD_METADATA_SZ ) ;
         ixmIndexNode *pTree =  NULL ;
         rc = index.getTree( pTree ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get index tree, rc: %d", rc ) ;
         if( FALSE == _dmsIndexCoverRecordBuilder::buildObj( pTree, container, builder ) )
         {
            goto done ;
         }
         builder.done() ;

         pNewRecord = (dmsRecord *)recordPtr ;
         pNewRecord->setNormal() ;
         pNewRecord->resetAttr() ;
         pNewRecord->setSize( DMS_RECORD_METADATA_SZ + builder.len() ) ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDWARNING, "Failed to build record, occur exception: %s",
                 e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSIXSCAN__BUILDIDINDEXRECORD, rc ) ;
      return (const CHAR *)( pNewRecord ) ;

   error:
      pNewRecord = nullptr ;
      goto done ;
   }

   /*
      _dmsEntireScanner implement
    */
   _dmsEntireScanner::_dmsEntireScanner( dmsStorageDataCommon *su,
                                         dmsMBContext *context,
                                         mthMatchRuntime *matchRuntime,
                                         dmsSecScanner &secScanner,
                                         rtnScanner *scanner,
                                         BOOLEAN ownedScanner,
                                         dmsScannerContext &scannerContext,
                                         DMS_ACCESS_TYPE accessType,
                                         INT64 maxRecords,
                                         INT64 skipNum,
                                         INT32 flag,
                                         IDmsOprHandler *opHandler )
   : _dmsScanner( su, context, matchRuntime, scanner->getEDUCB(), accessType, maxRecords, skipNum, flag, opHandler ),
     _secScanner( secScanner ),
     _scanner( scanner ),
     _scannerContext( scannerContext )
   {
      _firstRun      = TRUE ;
      _scanner       = scanner ;
      _ownedScanner  = ownedScanner ;

      _lockInited    = FALSE ;
      _isolation     = TRANS_ISOLATION_RU ;
      _lockType      = DPS_TRANSLOCK_MAX ;
      _lockOpMode    = DPS_TRANSLOCK_OP_MODE_ACQUIRE ;
   }

   _dmsEntireScanner::~_dmsEntireScanner()
   {
      if ( _ownedScanner )
      {
         SDB_OSS_DEL _scanner ;
         _scanner = NULL ;
      }
   }

   INT32 _dmsEntireScanner::_firstInit()
   {
      INT32 rc = SDB_OK ;

      if ( _lockInited )
      {
         _secScanner.initLockInfo( _isolation, _lockType, _lockOpMode ) ;
      }

      if ( !_context->isMBLock( _secScanner.getMBLockType() ) )
      {
         rc = _context->mbLock( _secScanner.getMBLockType() ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb lock failed, rc: %d", rc ) ;
      }

      rc = _onInit() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init scanner, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsEntireScanner::_pauseInnerScanner()
   {
      _secScanner.pause() ;
   }

   INT32 _dmsEntireScanner::advance( dmsRecordID &recordID,
                                     _mthRecordGenerator &generator,
                                     pmdEDUCB *cb,
                                     _mthMatchTreeContext *mthContext )
   {
      INT32 rc = SDB_OK ;

      if ( _firstRun )
      {
         rc = _firstInit() ;
         PD_RC_CHECK( rc, PDERROR, "First init failed, rc: %d", rc ) ;
         _firstRun = FALSE ;
      }

      while ( TRUE )
      {
         rc = _secScanner.advance( recordID, generator, cb, mthContext ) ;
         if ( SDB_DMS_EOC == rc )
         {
            if ( _secScanner.isHitEnd() )
            {
               goto error ;
            }

            // just pause
            _pauseInnerScanner() ;

            rc = SDB_OK ;
            continue ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to advance scanner, rc: %d", rc ) ;
            goto error ;
         }

         break ;
      }

   done:
      _saveAdvancedRecrodID( recordID, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _dmsEntireScanner::stop()
   {
      _secScanner.stop() ;
   }

   /*
      _dmsTBScanner implement
   */
   _dmsTBScanner::_dmsTBScanner( dmsStorageDataCommon *su,
                                 dmsMBContext *context,
                                 mthMatchRuntime *matchRuntime,
                                 rtnTBScanner *scanner,
                                 BOOLEAN ownedScanner,
                                 DMS_ACCESS_TYPE accessType,
                                 INT64 maxRecords,
                                 INT64 skipNum,
                                 INT32 flag,
                                 IDmsOprHandler *opHandler )
   : _dmsEntireScanner( su, context, matchRuntime, _secScanner, scanner,
                        ownedScanner, _scannerContext, accessType, maxRecords,
                        skipNum, flag, opHandler ),
     _secScanner( su, context, scanner, matchRuntime, accessType, maxRecords,
                  skipNum, flag, opHandler ),
     _scannerContext( &_secScanner )
   {
   }

   INT32 _dmsTBScanner::_onInit()
   {
      INT32 rc = SDB_OK ;

      _context->mbStat()->_crudCB.increaseTbScan( 1 ) ;

      return rc ;
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
                                 INT32 flag,
                                 IDmsOprHandler *opHandler )
   : _dmsEntireScanner( su, context, matchRuntime, _secScanner, scanner,
                        ownedScanner, _scannerContext, accessType, maxRecords, skipNum, flag,
                        opHandler ),
     _secScanner( su, context, scanner, matchRuntime, accessType, maxRecords,
                  skipNum, flag, opHandler ),
     _scannerContext( &_secScanner )
   {
   }

   INT32 _dmsIXScanner::_onInit()
   {
      INT32 rc = SDB_OK ;

      _context->mbStat()->_crudCB.increaseIxScan( 1 ) ;

      return rc ;
   }

}


