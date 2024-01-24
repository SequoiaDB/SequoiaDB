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

   Source File Name = dmsScanner.hpp

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
#ifndef DMSSCANNER_HPP__
#define DMSSCANNER_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "dms.hpp"
#include "dmsExtent.hpp"
#include "ossTypes.h"
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "dmsStorageBase.hpp"
#include "dmsStorageDataCommon.hpp"
#include "dmsStorageData.hpp"
#include "dmsStorageDataCapped.hpp"
#include "../bson/bson.h"
#include "../bson/bsonobj.h"
#include "mthMatchRuntime.hpp"
#include "ossMemPool.hpp"
#include "dmsTransLockCallback.hpp"
#include "dmsTransContext.hpp"
#include "dmsOprHandler.hpp"

using namespace bson ;

namespace engine
{

   // forward declaration
   class _dmsMBContext ;
   class _dmsStorageDataCommon ;
   class _dmsStorageData ;
   class _dmsStorageDataCapped ;
   class _mthMatchTreeContext ;
   class _rtnTBScanner ;
   class _rtnScanner ;
   class _rtnIXScanner ;
   class _pmdEDUCB ;
   class _monAppCB ;
   class dpsTransCB ;
   class _dpsTransExecutor;
   class _dmsSecScanner ;
   class _dmsScanner ;
   class _dmsTBScanner ;

   #define DMS_IS_WRITE_OPR(accessType)   \
      ( DMS_ACCESS_TYPE_UPDATE == accessType || \
        DMS_ACCESS_TYPE_DELETE == accessType ||\
        DMS_ACCESS_TYPE_INSERT == accessType )

   #define DMS_IS_READ_OPR(accessType) \
      ( DMS_ACCESS_TYPE_QUERY == accessType || \
        DMS_ACCESS_TYPE_FETCH == accessType )

   /*
      _dmsScannerContext define
    */
   class _dmsScannerContext : public _IContext
   {
   public:
      _dmsScannerContext( _dmsSecScanner *scanner ) ;
      virtual ~_dmsScannerContext() ;

   public:
      virtual INT32 pause() ;
      virtual INT32 resume() ;

   private:
      BOOLEAN _hasPaused ;
      _dmsSecScanner *_scanner ;
   } ;

   typedef class _dmsScannerContext dmsScannerContext ;
   typedef class _dmsScannerContext dmsTBScannerContext ;
   typedef class _dmsScannerContext dmsIXScannerContext ;

   /*
      _dmsScanner define
   */
   class _dmsScanner : public utilPooledObject
   {
      public:
         _dmsScanner ( _dmsStorageDataCommon *su,
                       _dmsMBContext *context,
                       mthMatchRuntime *matchRuntime,
                       _pmdEDUCB *cb,
                       DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                       INT64 maxRecords = -1,
                       INT64 skipNum = 0,
                       INT32 flags = 0,
                       IDmsOprHandler *opHandler = NULL ) ;
         virtual ~_dmsScanner () ;

         BOOLEAN  isReadOnly() const
         {
            return DMS_IS_WRITE_OPR( _accessType ) ? FALSE : TRUE ;
         }

         virtual dmsTransLockCallback*       callbackHandler() = 0 ;
         virtual const dmsTransRecordInfo*   recordInfo() const = 0 ;

      public:
         virtual INT32 advance ( dmsRecordID &recordID,
                                 _mthRecordGenerator &generator,
                                 _pmdEDUCB *cb,
                                 _mthMatchTreeContext *mthContext = NULL ) = 0 ;
         virtual void  stop () = 0 ;

         virtual dmsScannerContext *getScannerContext() = 0 ;

         const dmsRecordID &getAdvancedRecordID()
         {
            return _advancedRecordID ;
         }

         INT32 getMBLockType() const
         {
            return _mbLockType ;
         }

         virtual void initLockInfo( INT32 isolation,
                                    DPS_TRANSLOCK_TYPE lockType,
                                    DPS_TRANSLOCK_OP_MODE_TYPE lockOpMode ) = 0 ;

         virtual void enableCountMode() {}

         INT64 getMaxRecords() const { return _maxRecords ; }
         INT64 getSkipNum () const { return _skipNum ; }

      protected:
         void _saveAdvancedRecrodID( const dmsRecordID &recordID, INT32 rc ) ;
         void _checkMaxRecordsNum( _mthRecordGenerator &generator ) ;

      protected:
         _dmsStorageDataCommon  *_pSu ;
         _dmsMBContext          *_context ;
         mthMatchRuntime        *_matchRuntime ;
         DMS_ACCESS_TYPE         _accessType ;
         INT32                   _mbLockType ;

         dmsRecordID             _advancedRecordID ;
         IDmsOprHandler         *_opHandler ;

         INT64                   _maxRecords ;
         INT64                   _skipNum ;
         INT32                   _flags ;
   } ;
   typedef _dmsScanner dmsScanner ;

   /*
      _dmsScannerLockHandler define
    */
   class _dmsScannerLockHandler
   {
   public:
      _dmsScannerLockHandler( IDmsOprHandler *opHandler, INT32 flags ) ;
      virtual ~_dmsScannerLockHandler() ;

   protected:
      INT32 _acquireCSCLLock( _dmsStorageDataCommon *su,
                              _dmsMBContext *mbContext,
                              pmdEDUCB *cb,
                              IContext *transContext ) ;
      void  _releaseCSCLLock( _dmsStorageDataCommon *su,
                              _dmsMBContext *mbContext,
                              pmdEDUCB *cb ) ;

      void _initLockInfo( _dmsStorageDataCommon *su,
                          _dmsMBContext *mbContext,
                          DMS_ACCESS_TYPE accessType,
                          pmdEDUCB *cb ) ;

      void _initLockInfo( INT32 isolation,
                          DPS_TRANSLOCK_TYPE lockType,
                          DPS_TRANSLOCK_OP_MODE_TYPE lockOpMode ) ;

      INT32 _checkTransLock( _dmsStorageDataCommon *su,
                             _dmsMBContext *mbContext,
                             const dmsRecordID &curRID,
                             pmdEDUCB *cb,
                             dmsScanTransContext *transContext,
                             dmsRecordRW &recordRW,
                             dmsRecordID &waitUnlockRID,
                             BOOLEAN &skipRecord ) ;

      void _releaseTransLock( _dmsStorageDataCommon *su,
                              _dmsMBContext *mbContext,
                              const dmsRecordID &curRID,
                              pmdEDUCB *cb ) ;

      void _releaseAllLocks( _dmsStorageDataCommon *su,
                              _dmsMBContext *mbContext,
                              const dmsRecordID &curRID,
                              pmdEDUCB *cb ) ;

      virtual void _onRecordSkipped( const dmsRecordID &curRID,
                                     dmsScanTransContext *transContext )
      {
      }

      virtual void _onRecordLocked( const dmsRecordID &curRID,
                                    dmsScanTransContext *transContext,
                                    BOOLEAN &skipRecord )
      {
      }

   protected:
      // lock info
      BOOLEAN                 _isInited ;
      dpsTransCB             *_pTransCB ;
      INT32                   _transIsolation ;
      BOOLEAN                 _waitLock ;
      BOOLEAN                 _useRollbackSegment ;
      BOOLEAN                 _needEscalation ;
      BOOLEAN                 _hasLockedRecord ;
      INT8                    _recordLock ;
      INT8                    _selectLockMode ;
      INT8                    _lockOpMode ;
      BOOLEAN                 _needUnLock ;
      BOOLEAN                 _CSCLLockHeld ;
      dmsTransLockCallback    _callback ;
   } ;

   typedef class _dmsScannerLockHandler dmsScannerLockHandler ;

   /*
      _dmsSecScanner define
    */
   class _dmsSecScanner : public _dmsScanner, public _dmsScannerLockHandler
   {
   public:
      _dmsSecScanner( _dmsStorageDataCommon *su,
                      _dmsMBContext *context,
                      _rtnScanner *scanner,
                      mthMatchRuntime *matchRuntime,
                      DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                      INT64 maxRecords = -1,
                      INT64 skipNum = 0,
                      INT32 flag = 0,
                      IDmsOprHandler *handler = NULL ) ;
      virtual ~_dmsSecScanner() ;

      const dmsRecordID &getCurRID() const
      {
         return _curRID ;
      }

      virtual INT32 advance( dmsRecordID &recordID,
                             _mthRecordGenerator &generator,
                             _pmdEDUCB *cb,
                             _mthMatchTreeContext *mthContext = NULL ) ;
      virtual void pause() ;
      virtual void stop() ;

      virtual dmsTransLockCallback *callbackHandler() ;
      virtual const dmsTransRecordInfo *recordInfo() const ;

      virtual void initLockInfo( INT32 isolation,
                                 DPS_TRANSLOCK_TYPE lockType,
                                 DPS_TRANSLOCK_OP_MODE_TYPE lockOpMode )
      {
         _initLockInfo( isolation, lockType, lockOpMode ) ;
      }

      virtual void enableCountMode()
      {
         _isCountOnly = TRUE ;
      }

      virtual dmsScannerContext* getScannerContext()
      {
         return &_scannerContext ;
      }

      BOOLEAN isHitEnd() const ;

      _pmdEDUCB *getEDUCB()
      {
         return _cb ;
      }

      _rtnScanner *getScanner()
      {
         return _scanner ;
      }

      _dmsStorageDataCommon *getDataSU()
      {
         return _pSu ;
      }

      _dmsMBContext *getMBContext()
      {
         return _context ;
      }

   protected:
      INT32 _firstInit( pmdEDUCB *cb ) ;
      INT32 _fetchNext( dmsRecordID &recordID,
                        _mthRecordGenerator &generator,
                        _pmdEDUCB *cb,
                        _mthMatchTreeContext *mthContext = NULL ) ;

      virtual INT32 _onFirstInit( _pmdEDUCB *cb ) = 0 ;
      virtual INT32 _advanceScanner( _pmdEDUCB *cb, dmsRecordID &rid ) = 0 ;
      virtual INT32 _checkSnapshotID( BOOLEAN &isSnapshotSame ) = 0 ;
      virtual INT32 _getCurrentRID( dmsRecordID &nextRID ) = 0 ;
      virtual INT32 _getCurrentRecord( dmsRecordData &recordData ) = 0 ;

      virtual UINT64 _getOnceRestNum() const = 0 ;

      // for _dmsScannerLockHandler
      virtual void _onRecordSkipped( const dmsRecordID &curRID,
                                     dmsScanTransContext *transContext ) ;
      virtual void _onRecordLocked( const dmsRecordID &curRID,
                                    dmsScanTransContext *transContext,
                                    BOOLEAN &skipRecord ) ;

   protected:
      _rtnScanner          *_scanner ;
      dmsScanTransContext  _transContext ;
      dmsScannerContext    _scannerContext ;
      dmsRecordID          _curRID ;
      BOOLEAN              _isCountOnly ;
      BOOLEAN              _firstRun ;
      UINT64               _onceRestNum ;
      _pmdEDUCB            *_cb ;
      dmsRecordData        _recordData ;
   } ;
   typedef class _dmsSecScanner dmsSecScanner ;

   /*
      _dmsDataScanner define
    */
   class _dmsDataScanner : public _dmsSecScanner
   {
   public:
      _dmsDataScanner( _dmsStorageDataCommon *su,
                       _dmsMBContext *context,
                       _rtnTBScanner *scanner,
                       mthMatchRuntime *matchRuntime,
                       DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                       INT64 maxRecords = -1,
                       INT64 skipNum = 0,
                       INT32 flags = 0,
                       IDmsOprHandler *opHandler = NULL ) ;
      virtual ~_dmsDataScanner() = default ;

   protected:
      virtual INT32 _onFirstInit( _pmdEDUCB *cb ) ;
      virtual INT32 _advanceScanner( _pmdEDUCB *cb, dmsRecordID &rid ) ;
      virtual INT32 _checkSnapshotID( BOOLEAN &isSnapshotSame ) ;
      virtual INT32 _getCurrentRID( dmsRecordID &nextRID ) ;
      virtual INT32 _getCurrentRecord( dmsRecordData &recordData ) ;

      virtual UINT64 _getOnceRestNum() const ;

   protected:
      _rtnTBScanner *_scanner ;
   } ;

   typedef class _dmsDataScanner dmsDataScanner ;

   /*
      _dmsIndexScanner define
    */
   class _dmsIndexScanner : public _dmsSecScanner
   {
   public:
      _dmsIndexScanner( _dmsStorageDataCommon *su,
                        _dmsMBContext *context,
                        _rtnIXScanner *scanner,
                        mthMatchRuntime *matchRuntime,
                        DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                        INT64 maxRecords = -1,
                        INT64 skipNum = 0,
                        INT32 flags = 0,
                        IDmsOprHandler *opHandler = NULL ) ;
      virtual ~_dmsIndexScanner() = default ;

   protected:
      virtual INT32 _onFirstInit( _pmdEDUCB *cb ) ;

      virtual INT32 _advanceScanner( _pmdEDUCB *cb, dmsRecordID &rid ) ;
      virtual INT32 _checkSnapshotID( BOOLEAN &isSnapshotSame ) ;
      virtual INT32 _getCurrentRID( dmsRecordID &nextRID ) ;
      virtual INT32 _getCurrentRecord( dmsRecordData &recordData ) ;

      virtual UINT64 _getOnceRestNum() const ;

      const CHAR *_buildIndexRecord() ;

   protected:
      _rtnIXScanner *_scanner ;
   } ;

   typedef class _dmsIndexScanner dmsIndexScanner ;

   /*
      _dmsEntireScanner define
    */
   class _dmsEntireScanner : public _dmsScanner
   {
   public:
      _dmsEntireScanner( _dmsStorageDataCommon *su,
                         _dmsMBContext *context,
                         mthMatchRuntime *matchRuntime,
                         dmsSecScanner &secScanner,
                         _rtnScanner *scanner,
                         BOOLEAN ownedScanner,
                         dmsScannerContext &scannerContext,
                         DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                         INT64 maxRecords = -1,
                         INT64 skipNum = 0,
                         INT32 flag = 0,
                         IDmsOprHandler *opHandler = NULL ) ;
      virtual ~_dmsEntireScanner() ;

      virtual dmsTransLockCallback *callbackHandler()
      {
         return _secScanner.callbackHandler() ;
      }

      virtual const dmsTransRecordInfo *recordInfo() const
      {
         return _secScanner.recordInfo() ;
      }

      virtual INT32 advance( dmsRecordID &recordID,
                             _mthRecordGenerator &generator,
                             _pmdEDUCB *cb,
                             _mthMatchTreeContext *mthContext = NULL ) ;
      virtual void  stop() ;

      virtual dmsScannerContext *getScannerContext()
      {
         return &_scannerContext ;
      }

      virtual void initLockInfo( INT32 isolation,
                                 DPS_TRANSLOCK_TYPE lockType,
                                 DPS_TRANSLOCK_OP_MODE_TYPE lockOpMode )
      {
         if ( !_lockInited )
         {
            _isolation = isolation ;
            _lockType = lockType ;
            _lockOpMode = lockOpMode ;
            _lockInited = TRUE ;
         }
         _secScanner.initLockInfo( isolation, lockType, lockOpMode ) ;
      }

      _rtnScanner *getScanner()
      {
         return _scanner ;
      }

   protected:
      void  _pauseInnerScanner() ;
      INT32 _firstInit() ;

      virtual INT32 _onInit() = 0 ;

   protected:
      dmsSecScanner &            _secScanner ;
      _rtnScanner *              _scanner ;
      BOOLEAN                    _ownedScanner ;
      BOOLEAN                    _firstRun ;
      dmsScannerContext &        _scannerContext ;

      BOOLEAN                    _lockInited ;
      INT32                      _isolation ;
      DPS_TRANSLOCK_TYPE         _lockType ;
      DPS_TRANSLOCK_OP_MODE_TYPE _lockOpMode ;
   } ;

   typedef class _dmsEntireScanner dmsEntireScanner ;

   /*
      _dmsTBScanner define
    */
   class _dmsTBScanner : public _dmsEntireScanner
   {
   public:
      _dmsTBScanner( _dmsStorageDataCommon *su,
                     _dmsMBContext *context,
                     mthMatchRuntime *matchRuntime,
                     _rtnTBScanner *scanner,
                     BOOLEAN ownedScanner = TRUE,
                     DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                     INT64 maxRecords = -1,
                     INT64 skipNum = 0,
                     INT32 flag = 0,
                     IDmsOprHandler *opHandler = NULL ) ;
      virtual ~_dmsTBScanner() = default ;

   protected:
      virtual INT32 _onInit() ;

   private:
      dmsDataScanner _secScanner ;
      dmsTBScannerContext _scannerContext ;
   } ;

   typedef class _dmsTBScanner dmsTBScanner ;

   /*
      _dmsIXScanner define
   */
   class _dmsIXScanner : public _dmsEntireScanner
   {
   public:
      _dmsIXScanner( dmsStorageDataCommon *su,
                     _dmsMBContext *context,
                     mthMatchRuntime *matchRuntime,
                     _rtnIXScanner *scanner,
                     BOOLEAN ownedScanner = FALSE,
                     DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                     INT64 maxRecords = -1,
                     INT64 skipNum = 0,
                     INT32 flag = 0,
                     IDmsOprHandler *opHandler = NULL ) ;
      ~_dmsIXScanner() = default ;

   protected:
      INT32 _onInit() ;

   private:
      dmsIndexScanner _secScanner ;
      dmsIXScannerContext _scannerContext ;
   } ;
   typedef class _dmsIXScanner dmsIXScanner ;

   /*
      _IDmsScannerChecker define
    */
   // scanner checker to check if scanner is interrupted
   class _IDmsScannerChecker
   {
   public:
      _IDmsScannerChecker() {}
      virtual ~_IDmsScannerChecker() {}

   public:
      virtual BOOLEAN needInterrupt() = 0 ;
   } ;
   typedef class _IDmsScannerChecker IDmsScannerChecker ;

   /*
      _IDmsScannerCheckerCreator define
    */
   class _IDmsScannerCheckerCreator
   {
   private:
      // disallow copy and assign
      _IDmsScannerCheckerCreator( const _IDmsScannerCheckerCreator& ) ;
      void operator=( const _IDmsScannerCheckerCreator & ) ;

   protected:
      _IDmsScannerCheckerCreator() {}

   public:
      virtual ~_IDmsScannerCheckerCreator() {}
      virtual INT32 createChecker( UINT32 suLID,
                                   UINT32 mbLID,
                                   const CHAR *csName,
                                   const CHAR *clShortName,
                                   const CHAR *optrDesc,
                                   _pmdEDUCB *cb,
                                   IDmsScannerChecker **ppChecker ) = 0 ;
      virtual void releaseChecker( IDmsScannerChecker *pChecker ) = 0 ;
   } ;
   typedef class _IDmsScannerCheckerCreator IDmsScannerCheckerCreator ;

}

#endif //DMSSCANNER_HPP__

