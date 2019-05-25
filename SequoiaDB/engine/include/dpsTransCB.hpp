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
#include "ossLatch.hpp"
#include "dms.hpp"
#include "dpsTransLock.hpp"
#include "dpsTransLockBucket.hpp"
#include "dpsLogRecord.hpp"
#include "sdbInterface.hpp"
#include "ossEvent.hpp"

namespace engine
{
   #define DPS_TRANSID_ROLLBACKTAG_BIT          0X0000800000000000ll
   #define DPS_TRANSID_FIRSTOP_BIT              0X0000400000000000ll
   #define DPS_TRANSID_SN_BIT                   0X000000FFFFFFFFFFll
   #define DPS_TRANSID_VALID_BIT                0XFFFF00FFFFFFFFFFll

   class dpsTransLock;
   class _pmdEDUCB;
   class _dmsExtScanner ;
   class _dmsIXSecScanner ;

   typedef std::map<DPS_TRANS_ID, DPS_LSN_OFFSET>  TRANS_MAP;
   typedef std::map<DPS_TRANS_ID, _pmdEDUCB * >    TRANS_CB_MAP;
   typedef std::map<DPS_LSN_OFFSET, DPS_TRANS_ID>  TRANS_LSN_ID_MAP;
   typedef std::map<DPS_TRANS_ID, DPS_LSN_OFFSET>  TRANS_ID_LSN_MAP;
   typedef std::queue< EDUID >                     TRANS_EDU_LIST ;

   class _monTransInfo : public SDBObject
   {
   public:
      DPS_TRANS_ID         _transID ;
      DPS_LSN_OFFSET       _curTransLsn ;
      UINT64               _eduID ;
      UINT64               _relatedNID ;
      UINT32               _relatedTID ;
      UINT32               _locksNum ;
      dpsTransLockId       _waitLock ;
      DpsTransCBLockList   _lockList ;

   } ;
   typedef class _monTransInfo monTransInfo ;


   /*
      dpsTransCB define
   */
   class dpsTransCB : public _IControlBlock, public _IEventHander
   {
      friend class _dmsExtScannerBase ;
      friend class _dmsExtScanner ;
      friend class _dmsIXSecScanner ;
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

      /*
      *TransactionID:
      +---------------+-----------+-----------+
      | nodeID(16bit) | TAG(8bit) | SN(40bit) |
      +---------------+-----------+-----------+
      */
      DPS_TRANS_ID allocTransID() ;
      DPS_TRANS_ID getRollbackID( DPS_TRANS_ID transID ) ;
      DPS_TRANS_ID getTransID( DPS_TRANS_ID rollbackID ) ;

      BOOLEAN isRollback( DPS_TRANS_ID transID ) ;
      BOOLEAN isFirstOp( DPS_TRANS_ID transID );
      void    clearFirstOpTag( DPS_TRANS_ID &transID );

      INT32 startRollbackTask() ;
      INT32 stopRollbackTask() ;
      BOOLEAN isDoRollback() const { return _doRollback ; }
      INT32   waitRollback( UINT64 millicSec = -1 ) ;

      void addTransInfo( DPS_TRANS_ID transID, DPS_LSN_OFFSET lsnOffset );
      void updateTransInfo( DPS_TRANS_ID transID, DPS_LSN_OFFSET lsnOffset );

      void addTransCB( DPS_TRANS_ID transID, _pmdEDUCB *eduCB ) ;
      void delTransCB( DPS_TRANS_ID transID ) ;
      void dumpTransEDUList( TRANS_EDU_LIST  &eduList ) ;
      UINT32 getTransCBSize() ;
      void termAllTrans() ;
      TRANS_MAP *getTransMap() ;

      void clearTransInfo();

      void saveTransInfoFromLog( const dpsLogRecord &record ) ;
      BOOLEAN rollbackTransInfoFromLog( const dpsLogRecord &record ) ;

      void addBeginLsn( DPS_LSN_OFFSET beginLsn, DPS_TRANS_ID transID ) ;
      void delBeginLsn( DPS_TRANS_ID transID ) ;
      DPS_LSN_OFFSET getBeginLsn( DPS_TRANS_ID transID ) ;
      DPS_LSN_OFFSET getOldestBeginLsn();

      BOOLEAN isNeedSyncTrans();
      void setIsNeedSyncTrans( BOOLEAN isNeed );

      INT32 syncTransInfoFromLocal( DPS_LSN_OFFSET beginLsn );

      INT32 transLockGetX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID = DMS_INVALID_MBID,
                           const dmsRecordID *recordID = NULL );

      INT32 transLockGetS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID = DMS_INVALID_MBID,
                           const dmsRecordID *recordID = NULL );

      INT32 transLockGetIX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                            UINT16 collectionID = DMS_INVALID_MBID );

      INT32 transLockGetIS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                            UINT16 collectionID = DMS_INVALID_MBID );

      void transLockRelease( _pmdEDUCB *eduCB, UINT32 logicCSID,
                             UINT16 collectionID = DMS_INVALID_MBID,
                             const dmsRecordID *recordID = NULL );

      void transLockReleaseAll( _pmdEDUCB *eduCB ) ;

      BOOLEAN isTransOn() ;

      INT32 transLockTestS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                            UINT16 collectionID = DMS_INVALID_MBID,
                            const dmsRecordID *recordID = NULL );

      INT32 transLockTestX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                            UINT16 collectionID = DMS_INVALID_MBID,
                            const dmsRecordID *recordID = NULL );

      INT32 transLockTryX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID = DMS_INVALID_MBID,
                           const dmsRecordID *recordID = NULL );

      INT32 transLockTryS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID = DMS_INVALID_MBID,
                           const dmsRecordID *recordID = NULL );

      BOOLEAN hasWait( UINT32 logicCSID, UINT16 collectionID,
                       const dmsRecordID *recordID);

      INT32 reservedLogSpace( UINT32 length, _pmdEDUCB *cb ) ;

      void releaseLogSpace( UINT32 length, _pmdEDUCB *cb );

      UINT64 remainLogSpace();

      UINT64 usedLogSpace();

   protected:
      INT32 tryOrAppendX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                          UINT16 collectionID, const dmsRecordID *recordID );

      INT32 waitLock( _pmdEDUCB *eduCB, UINT32 logicCSID,
                      UINT16 collectionID, const dmsRecordID *recordID ) ;
   private:
      DPS_TRANS_ID      _TransIDH16 ;
      ossAtomic64       _TransIDL48Cur ;
      ossSpinXLatch     _MapMutex ;
      TRANS_MAP         _TransMap ;
      ossSpinXLatch     _CBMapMutex ;
      TRANS_CB_MAP      _cbMap ;
      dpsTransLock      _TransLock ;
      BOOLEAN           _isOn ;
      BOOLEAN           _doRollback ;
      ossEvent          _rollbackEvent ;
      ossSpinXLatch     _lsnMapMutex ;
      TRANS_LSN_ID_MAP  _beginLsnIdMap ;
      TRANS_ID_LSN_MAP  _idBeginLsnMap ;
      BOOLEAN           _isNeedSyncTrans ;
      ossSpinXLatch     _maxFileSizeMutex ;
      UINT64            _maxUsedSize ;
      UINT64            _logFileTotalSize ;
      UINT64            _accquiredSpace ;

      TRANS_ID_LSN_MAP  _rollbackInfo ;

   } ;

   /*
      get global cb obj
   */
   dpsTransCB* sdbGetTransCB () ;

}

#endif // DPSTRANSCB_HPP_
