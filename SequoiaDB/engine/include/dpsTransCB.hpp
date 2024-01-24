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

   Source File Name = dpsTransCB.hpp

   Descriptive Name = Operating System Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSTRANSCB_HPP_
#define DPSTRANSCB_HPP_

#include <queue>
#include "oss.hpp"
#include "ossAtomic.hpp"
#include "dpsDef.hpp"
#include "monLatch.hpp"
#include "dms.hpp"
#include "dpsTransLockMgr.hpp"
#include "dpsLogRecord.hpp"
#include "sdbInterface.hpp"
#include "ossEvent.hpp"
#include "ossMemPool.hpp"
#include "monLatch.hpp"
#include "../bson/bson.hpp"

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;
   class oldVersionCB ;
   class dpsTransLockManager ;
   class _dpsITransLockCallback ;
   class _dpsLogWrapper ;

   /*
      _dpsTransPendingKey define
   */
   struct _dpsTransPendingKey
   {
      ossPoolString  _collection ;
      BSONObj        _obj ;

      _dpsTransPendingKey()
      {
      }

      _dpsTransPendingKey( const CHAR *collection,
                           const BSONObj &obj,
                           BOOLEAN getOwned )
      {
         setKey( collection, obj, getOwned ) ;
      }

      bool operator <( const _dpsTransPendingKey &rhs ) const
      {
         /// compare id
         BSONElement l, r ;
         INT32 res = _collection.compare( rhs._collection ) ;
         if ( res < 0 )
         {
            return true ;
         }
         else if ( res > 0 )
         {
            return false ;
         }
         l = _obj.getField( DMS_ID_KEY_NAME ) ;
         r = rhs._obj.getField( DMS_ID_KEY_NAME ) ;
         return l.woCompare( r, FALSE ) < 0 ;
      }

      bool operator ==( const _dpsTransPendingKey &rhs ) const
      {
         if ( _collection == rhs._collection )
         {
            /// compare id
            BSONElement l, r ;
            l = _obj.getField( DMS_ID_KEY_NAME ) ;
            r = rhs._obj.getField( DMS_ID_KEY_NAME ) ;
            return l.woCompare( r, FALSE ) == 0 ;
         }
         return false ;
      }

      void setKey( const CHAR *collection,
                   const BSONObj &obj,
                   BOOLEAN getOwned )
      {
         _collection.assign( collection ) ;
         if ( getOwned )
         {
            _obj = obj.getOwned() ;
         }
         else
         {
            _obj = obj ;
         }
      }
   } ;
   typedef struct _dpsTransPendingKey dpsTransPendingKey ;

   /*
      _dpsTransPendingValue define
   */
   struct _dpsTransPendingValue
   {
      BSONObj     _obj ;
      INT32       _opType ;

      _dpsTransPendingValue()
      : _opType( LOG_TYPE_DUMMY )
      {
      }

      _dpsTransPendingValue( const BSONObj &obj, INT32 opType )
      {
         setValue( obj, opType ) ;
      }

      void setValue( const BSONObj &obj, INT32 opType )
      {
         _obj = obj.getOwned() ;
         _opType = opType ;
      }
   } ;
   typedef struct _dpsTransPendingValue dpsTransPendingValue ;

   // NOTE:
   // 1. pending key is expected DMS record during rollback
   //    it should contains the whole DMS record
   // 2. pending value is the actual DMS record on disk ( must have OID )
   //    during rollback
   typedef ossPoolMap<dpsTransPendingKey,
                      dpsTransPendingValue> MAP_TRANS_PENDING_OBJ ;

   // helper functions for transaction rollback pending objects
   // add pending key and value into pending map
   INT32 dpsAddTransPending( MAP_TRANS_PENDING_OBJ &pendingMap,
                             dpsTransPendingKey &pendingKey,
                             dpsTransPendingValue &pendingValue,
                             BOOLEAN &added ) ;
   // remove pending key and value from pending map
   // and get back the removed key and value if needed
   INT32 dpsRemoveTransPending( MAP_TRANS_PENDING_OBJ &pendingMap,
                                const dpsTransPendingKey &pendingKey,
                                BSONObj *oldKey,
                                dpsTransPendingValue *oldValue,
                                BOOLEAN &removed ) ;

   /*
      _dpsTransBackInfo define
   */
   struct _dpsTransBackInfo
   {
      DPS_LSN_OFFSET                _lsn ;
      INT32                         _status ;
      // current pending LSN: the last LSN still rollback pending
      DPS_LSN_OFFSET                _curLSNWithRBPending ;
      // current non pending LSN: the LSN which did not create pending object
      // during rollback pending
      ossPoolSet< DPS_LSN_OFFSET >  _curNonPendingLSN ;

      _dpsTransBackInfo( DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET,
                         INT32 status = DPS_TRANS_DOING )
      {
         _lsn = lsn ;
         _curLSNWithRBPending = DPS_INVALID_LSN_OFFSET ;
         _status = status ;
      }
   } ;
   typedef _dpsTransBackInfo dpsTransBackInfo ;

   typedef ossPoolMap<DPS_TRANS_ID, dpsTransBackInfo> TRANS_MAP ;
   typedef ossPoolMap<DPS_TRANS_ID, _pmdEDUCB * >     TRANS_CB_MAP ;
   typedef ossPoolMap<DPS_LSN_OFFSET, DPS_TRANS_ID>   TRANS_LSN_ID_MAP ;
   typedef ossPoolMap<DPS_TRANS_ID, DPS_LSN_OFFSET>   TRANS_ID_LSN_MAP ;
   typedef std::queue< EDUID >                        TRANS_EDU_LIST ;

   /*
      _dpsHisTransStatus define
   */
   struct _dpsHisTransStatus
   {
      INT32             _status ;
      DPS_LSN_OFFSET    _lsn ;

      _dpsHisTransStatus( INT32 status = DPS_TRANS_COMMIT,
                          DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET )
      {
         _status = status ;
         _lsn = lsn ;
      }
   } ;
   typedef _dpsHisTransStatus dpsHisTransStatus ;

   typedef ossPoolMap<DPS_TRANS_ID, dpsHisTransStatus>   TRANS_ID_2_STATUS ;

   // delta between runtime log and undo log due to TransRelatedLSN
   // sizeof( _dpsRecordEle ) + sizeof( UINT64 ) = 13
   // and align to 4 bytes, finally, we need at least 16 bytes
   #define DPS_TRANS_LOG_UNDO_DELTA  ( 16 )
   /*
      dpsTransCB define
   */
   class dpsTransCB : public _IControlBlock, public _IEventHander
   {
   public:
      dpsTransCB() ;
      virtual ~dpsTransCB() ;

      virtual SDB_CB_TYPE cbType() const ;
      virtual const CHAR* cbName() const ;

      virtual INT32  init () ;
      virtual INT32  active () ;
      virtual INT32  deactive () ;
      virtual INT32  fini () ;
      virtual void   onConfigChange() ;

      virtual void   onRegistered( const MsgRouteID &nodeID ) ;
      virtual void   onPrimaryChange( BOOLEAN primary,
                                      SDB_EVENT_OCCUR_TYPE occurType ) ;

      void           setEventHandler( dpsTransEvent *pEventHandler ) ;
      dpsTransEvent* getEventHandler() ;

      /*
      *TransactionID:
      +---------------+-----------+-----------+
      | nodeID(16bit) | TAG(8bit) | SN(40bit) |
      +---------------+-----------+-----------+
      */
      DPS_TRANS_ID allocTransID( BOOLEAN isAutoCommit = FALSE ) ;
      DPS_TRANS_ID getRollbackID( DPS_TRANS_ID transID ) ;
      DPS_TRANS_ID getTransID( DPS_TRANS_ID rollbackID ) ;

      oldVersionCB * getOldVCB () { return _oldVCB ; }

      BOOLEAN isRollback( DPS_TRANS_ID transID ) ;
      BOOLEAN isFirstOp( DPS_TRANS_ID transID ) ;
      void    clearFirstOpTag( DPS_TRANS_ID &transID ) ;

      // check transaction if rollback pending
      BOOLEAN isRBPending( DPS_TRANS_ID transID ) ;
      // check if has rollback pending transactions
      BOOLEAN hasRBPendingTrans() ;

      INT32 startRollbackTask() ;
      INT32 stopRollbackTask( UINT64 doRollbackID ) ;
      BOOLEAN isDoRollback() const { return _doRollback ; }
      INT32   waitRollback( UINT64 millicSec = -1 ) ;

      INT32 addTransInfo( DPS_TRANS_ID transID,
                          DPS_LSN_OFFSET lsnOffset,
                          INT32 status ) ;
      // NOTE: log rollback means rollbacked by consulting or replay failure
      void updateTransInfo( DPS_TRANS_ID transID,
                            DPS_LSN_OFFSET lsnOffset,
                            INT32 status ) ;
      void updateTransInfo( dpsTransBackInfo &transInfo,
                            INT32 status,
                            DPS_LSN_OFFSET lsn,
                            BOOLEAN rbPending ) ;

      void updateTransStatus( DPS_TRANS_ID transID,
                              INT32 status ) ;

      BOOLEAN  addTransCB( DPS_TRANS_ID transID, _pmdEDUCB *eduCB ) ;
      void     delTransCB( DPS_TRANS_ID transID ) ;
      void     dumpTransEDUList( TRANS_EDU_LIST  &eduList ) ;
      void     snapTransLockWaiterLRB( DPS_TX_WAIT_LRB_SET & txWaiterLRBSet ) ;
      UINT32   getTransCBSize() ;
      void     termAllTrans() ;
      TRANS_MAP *getTransMap() ;
      void     cloneTransMap( TRANS_MAP &result ) ;

      void     addHisTrans( DPS_TRANS_ID transID,
                            INT32 status,
                            DPS_LSN_OFFSET lsn ) ;
      void     delHisTrans( DPS_TRANS_ID transID ) ;
      void     clearHisTrans() ;
      void     clearOutDateHisTrans( DPS_LSN_OFFSET lsn ) ;
      INT32    checkTransStatus( DPS_TRANS_ID transID, DPS_LSN_OFFSET & lsn ) ;

      void     clearTransInfo() ;

      void     saveTransInfoFromLog( const dpsLogRecord &record ) ;
      // rollback transaction info to expect LSN ( generally it is older than
      // replayer's completed LSN )
      INT32    rollbackTransInfoFromLog( _dpsLogWrapper *dpsCB,
                                         const DPS_LSN &expectLSN ) ;
      // rollback transaction info for a single DPS record
      BOOLEAN  rollbackTransInfoFromLog( const dpsLogRecord &record ) ;

      void           addBeginLsn( DPS_LSN_OFFSET beginLsn, DPS_TRANS_ID transID ) ;
      void           delBeginLsn( DPS_TRANS_ID transID ) ;
      DPS_LSN_OFFSET getBeginLsn( DPS_TRANS_ID transID ) ;
      DPS_LSN_OFFSET getOldestBeginLsn() ;

      BOOLEAN  isNeedSyncTrans() ;
      void     setIsNeedSyncTrans( BOOLEAN isNeed ) ;

      INT32 syncTransInfoFromLocal( DPS_LSN_OFFSET beginLsn ) ;

      // get record-X-lock: also get the space-IS-lock and collection-IX-lock
      // get collection-X-lock: also get the space-IX-lock
      INT32 transLockGetX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID = DMS_INVALID_MBID,
                           const dmsRecordID *recordID = NULL,
                           _IContext *pContext = NULL,
                           dpsTransRetInfo * pdpsTxResInfo = NULL,
                           _dpsITransLockCallback *callback = NULL ) ;

      // get record-U-lock: also get the space-IS-lock and collection-IS-lock
      INT32 transLockGetU( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID,
                           const dmsRecordID *recordID,
                           _IContext *pContext = NULL,
                           dpsTransRetInfo * pdpsTxResInfo = NULL,
                           _dpsITransLockCallback *callback = NULL ) ;

      // get record-S-lock: also get the space-IS-lock and collection-IS-lock
      // get collection-S-lock: also get the space-IS-lock
      INT32 transLockGetS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID = DMS_INVALID_MBID,
                           const dmsRecordID *recordID = NULL,
                           _IContext *pContext = NULL,
                           dpsTransRetInfo * pdpsTxResInfo = NULL,
                           _dpsITransLockCallback *callback = NULL,
                           BOOLEAN useEscalation = TRUE ) ;

      // also get the space-IS-lock
      INT32 transLockGetIX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                            UINT16 collectionID = DMS_INVALID_MBID,
                            _IContext *pContext = NULL,
                            dpsTransRetInfo * pdpsTxResInfo = NULL ) ;

      // also get the space-IS-lock
      INT32 transLockGetIS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                            UINT16 collectionID = DMS_INVALID_MBID,
                            _IContext *pContext = NULL,
                            dpsTransRetInfo * pdpsTxResInfo = NULL ) ;

      // release record-lock: also release the space-lock and collection-lock
      // release collection-lock: also release the space-lock
      void transLockRelease( _pmdEDUCB *eduCB, UINT32 logicCSID,
                             UINT16 collectionID = DMS_INVALID_MBID,
                             const dmsRecordID *recordID = NULL,
                             _dpsITransLockCallback *callback = NULL,
                             BOOLEAN forceRelease = FALSE,
                             BOOLEAN releaseUpperLock = TRUE ) ;

      void transLockReleaseAll( _pmdEDUCB *eduCB,
                                _dpsITransLockCallback * callback = NULL ) ;

      BOOLEAN isTransOn() const ;

      // test if the lock can be got.
      // test record-S-lock: also test the space-IS-lock and collection-IS-lock
      // test collection-IS-lock: also test the space-IS-lock
      INT32 transLockTestS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                            UINT16 collectionID = DMS_INVALID_MBID,
                            const dmsRecordID *recordID = NULL,
                            dpsTransRetInfo * pdpsTxResInfo = NULL,
                            _dpsITransLockCallback *callback = NULL ) ;

      INT32 transLockTestSPreempt( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                   UINT16 collectionID = DMS_INVALID_MBID,
                                   const dmsRecordID *recordID = NULL,
                                   dpsTransRetInfo * pdpsTxResInfo = NULL,
                                   _dpsITransLockCallback *callback = NULL,
                                   BOOLEAN needUpperLock = TRUE ) ;

      INT32 transLockTestIS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                             UINT16 collectionID = DMS_INVALID_MBID,
                             const dmsRecordID *recordID = NULL,
                             dpsTransRetInfo * pdpsTxResInfo = NULL ) ;

      // test if the lock can be got.
      // test record-X-lock: also test the space-IS-lock and collection-IX-lock
      INT32 transLockTestX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                            UINT16 collectionID = DMS_INVALID_MBID,
                            const dmsRecordID *recordID = NULL,
                            dpsTransRetInfo * pdpsTxResInfo = NULL,
                            _dpsITransLockCallback *callback = NULL,
                            BOOLEAN needUpperLock = TRUE ) ;

      INT32 transLockTestIX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                             UINT16 collectionID = DMS_INVALID_MBID,
                             const dmsRecordID *recordID = NULL,
                             dpsTransRetInfo * pdpsTxResInfo = NULL ) ;

      // test if the lock can be got.
      // test record-U-lock: also test the space-IX-lock and collection-IX-lock
      INT32 transLockTestU( _pmdEDUCB *eduCB, UINT32 logicCSID,
                            UINT16 collectionID ,
                            const dmsRecordID *recordID,
                            dpsTransRetInfo * pdpsTxResInfo = NULL,
                            _dpsITransLockCallback *callback = NULL ) ;

      // test if the Z lock can be got.
      INT32 transLockTestZ( _pmdEDUCB *eduCB,
                            UINT32 logicCSID,
                            UINT16 collectionID ,
                            const dmsRecordID *recordID,
                            dpsTransRetInfo *pdpsTxResInfo = NULL,
                            _dpsITransLockCallback *callback = NULL ) ;

      // try to get record-X-lock: also try to get the space-IX-lock and
      // collection-IX-lock
      // try to get collection-X-lock: also try to get the space-IX-lock
      INT32 transLockTryX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID = DMS_INVALID_MBID,
                           const dmsRecordID *recordID = NULL,
                           dpsTransRetInfo * pdpsTxResInfo = NULL,
                           _dpsITransLockCallback * callback = NULL ) ;

      // try to get record-Z-lock: also try to get the space-IX-lock and
      // collection-IX-lock
      // try to get collection-Z-lock: also try to get the space-IX-lock
      INT32 transLockTryZ( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID = DMS_INVALID_MBID,
                           const dmsRecordID *recordID = NULL,
                           dpsTransRetInfo * pdpsTxResInfo = NULL,
                           _dpsITransLockCallback * callback = NULL ) ;

      // try to get record-U-lock: also try to get the space-IX-lock and
      // collection-IX-lock
      INT32 transLockTryU( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID ,
                           const dmsRecordID *recordID,
                           dpsTransRetInfo * pdpsTxResInfo = NULL,
                           _dpsITransLockCallback *callback = NULL ) ;

      // try to get record-S-lock: also try to get the space-IS-lock and
      // collection-IS-lock
      // try to get collection-S-lock: also try to get the space-IS-lock
      INT32 transLockTryS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID = DMS_INVALID_MBID,
                           const dmsRecordID *recordID = NULL,
                           dpsTransRetInfo * pdpsTxResInfo = NULL,
                           _dpsITransLockCallback * callback = NULL ) ;

      // try to get IS-lock
      INT32 transLockTryIS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                            UINT16 collectionID = DMS_INVALID_MBID,
                            dpsTransRetInfo * pdpsTxResInfo = NULL,
                            _dpsITransLockCallback * callback = NULL ) ;

      // check if any writing transactions on the object, and then try acquire
      // S lock
      INT32 transLockTrySAgainstWrite( _pmdEDUCB *eduCB,
                                       UINT32 logicCSID,
                                       UINT16 collectionID = DMS_INVALID_MBID,
                                       const dmsRecordID *recordID = NULL,
                                       dpsTransRetInfo *pdpsTxResInfo = NULL,
                                       _dpsITransLockCallback *callback = NULL ) ;

      // kill waiters for a specified lock ID
      BOOLEAN transLockKillWaiters( UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID,
                                    INT32 errorCode ) ;

      BOOLEAN transIsHolding( _pmdEDUCB *eduCB, UINT32 logicCSID,
                              UINT16 collectionID,
                              const dmsRecordID *recordID ) ;

      BOOLEAN hasWait( UINT32 logicCSID, UINT16 collectionID,
                       const dmsRecordID *recordID) ;

      INT32 getIncompTrans( _pmdEDUCB *               cb,
                            const dpsTransLockId &    lockID,
                            const DPS_TRANSLOCK_TYPE  lockMode,
                            BOOLEAN                   canSelfIncomp,
                            DPS_TRANS_ID_SET &        incompTrans ) ;

      INT32 reservedLogSpace( UINT32 length, _pmdEDUCB *cb ) ;

      void releaseLogSpace( UINT32 length, _pmdEDUCB *cb ) ;

      void releaseRBLogSpace( _pmdEDUCB *cb ) ;

      UINT64 remainLogSpace() ;

      UINT64 usedLogSpace() ;

      dpsTransLockManager * getLockMgrHandle() ;
      ixmIndexLockManager * getIndexLockMgrHandle() ;

      UINT32 getMaxLRSize() ;
      void   updateMaxLRSize( UINT32 recordSize, DPS_LSN_OFFSET curLSN ) ;
      void   printCounters() ;

      // succeed and error counts
      UINT64 getSucCount() const
      {
         return _sucCount ;
      }

      UINT64 getErrCount() const
      {
         return _errCount ;
      }

      void incSucCount()
      {
         ++ _sucCount ;
      }

      void incErrCount()
      {
         ++ _errCount ;
      }

   private:
      DPS_TRANS_ID      _TransIDH16 ;
      ossAtomic64       _TransIDL48Cur ;

      monSpinXLatch     _MapMutex ;
      TRANS_MAP         _TransMap ;

      monSpinXLatch     _CBMapMutex ;
      TRANS_CB_MAP      _cbMap ;

      BOOLEAN           _isOn ;
      BOOLEAN           _doRollback ;
      UINT64            _doRollbackID ;
      ossEvent          _rollbackEvent ;

      monSpinXLatch     _lsnMapMutex ;
      TRANS_LSN_ID_MAP  _beginLsnIdMap ;
      TRANS_ID_LSN_MAP  _idBeginLsnMap ;

      monSpinXLatch     _hisMutex ;
      TRANS_ID_2_STATUS _hisTransStatus ;
      TRANS_LSN_ID_MAP  _hisLsnTrans ;

      BOOLEAN           _isNeedSyncTrans ;
      UINT64            _logFileTotalSize ;

      // Largest two record size within the system, and the most recent LR LSN
      // used these two record. We need the largest record size to caculate if
      // we have sufficient log size for LR during reserveLogSpace
      UINT32            _maxLRSize1 ;
      UINT64            _maxLRLSN1 ;
      UINT32            _maxLRSize2 ;
      UINT64            _maxLRLSN2 ;
      // The _reservedRBspace and _reservedSpace are incremented by the size of
      // LR at runtime before writting LR. Once LR is written, _reservedSpace
      // is released.
      // each transaction also track total log space it reserves
      // for rollback. The space is released during commit or rollback.
      // _reservedRBSpace holds the sum of such space from all transactions
      ossAtomic64       _reservedRBSpace ;
      ossAtomic64       _reservedSpace ;

      dpsTransLockManager  *_transLockMgr ;
      ixmIndexLockManager  *_indexLockMgr ;
      oldVersionCB         *_oldVCB ;  // control block holding old(last committed)
                                       // version of record and index key value

      dpsTransEvent        *_pEventHandler ;

      // succeed and error counts
      // no need to be atomic
      volatile UINT64      _sucCount ;
      volatile UINT64      _errCount ;
   } ;

   /*
      get global cb obj
   */
   dpsTransCB* sdbGetTransCB () ;

}

#endif // DPSTRANSCB_HPP_
