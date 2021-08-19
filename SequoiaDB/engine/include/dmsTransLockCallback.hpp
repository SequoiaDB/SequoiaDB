/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = dmsTransLockCallback.hpp

   Descriptive Name = DMS Transaction Lock Callback Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/02/2019  CYX Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_TRANS_LOCK_CALLBACK_HPP__
#define DMS_TRANS_LOCK_CALLBACK_HPP__

#include "dpsTransLockCallback.hpp"
#include "dpsTransVersionCtrl.hpp"
#include "dmsOprHandler.hpp"
#include "dpsTransCB.hpp"
#include "pmdEDU.hpp"

using namespace bson ;

namespace engine
{

   class dpsTransCB ;
   class _dmsRecordRW ;
   class oldVersionContainer ;
   class oldVersionCB ;
   class _rtnIXScanner ;
   struct _dmsMBStatInfo ;

   // Class to implment lock call back funtions for DMS scanner
   class dmsTransLockCallback : public _dpsITransLockCallback,
                                public _IDmsOprHandler
   {
   public:
      dmsTransLockCallback() ;

      dmsTransLockCallback( dpsTransCB *transCB,
                            _pmdEDUCB  *eduCB ) ;

      void     clearStatus() ;

      virtual ~dmsTransLockCallback() ;

      void     setBaseInfo( dpsTransCB *transCB, _pmdEDUCB *eduCB ) ;

      void     setIDInfo( INT32 csID, UINT16 clID,
                          UINT32 csLID, UINT32 clLID ) ;

      void     setIXScanner( _rtnIXScanner *pScanner ) ;

      void     attachRecordRW( _dmsRecordRW * recordRW ) ;
      void     detachRecordRW() ;

      /*
         Status
      */
      BOOLEAN  isSkipRecord() const { return _skipRecord ; }
      INT32    getResult() const { return _result ; }
      BOOLEAN  hasError() const { return SDB_OK != _result ? TRUE : FALSE ; }
      BOOLEAN  isUseOldVersion() const { return _useOldVersion ; }

      const dmsTransRecordInfo*  getTransRecordInfo() const ;

   public:

      /// Interface
      virtual void afterLockAcquire( const dpsTransLockId &lockId,
                                     INT32 irc,
                                     DPS_TRANSLOCK_TYPE requestLockMode,
                                     UINT32 refCounter,
                                     DPS_TRANSLOCK_OP_MODE_TYPE opMode,
                                     const dpsTransLRBHeader *pLRBHeader,
                                     dpsLRBExtData *pExtData ) ;

      virtual void beforeLockRelease( const dpsTransLockId &lockId,
                                      DPS_TRANSLOCK_TYPE lockMode,
                                      UINT32 refCounter,
                                      const dpsTransLRBHeader *pLRBHeader,
                                      dpsLRBExtData *pExtData ) ;

   public:
      virtual void  onCSClosed( INT32 csID ) ;
      virtual void  onCLTruncated( INT32 csID, UINT16 clID ) ;

      virtual INT32 onCreateIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   _pmdEDUCB *cb ) ;

      virtual INT32 onDropIndex( _dmsMBContext *context,
                                 const ixmIndexCB *indexCB,
                                 _pmdEDUCB *cb ) ;

      virtual INT32 onRebuildIndex( _dmsMBContext *context,
                                    const ixmIndexCB *indexCB,
                                    _pmdEDUCB *cb,
                                    utilWriteResult *pResult ) ;

      virtual INT32 onInsertRecord( _dmsMBContext *context,
                                    const BSONObj &object,
                                    const dmsRecordID &rid,
                                    const _dmsRecordRW *pRecordRW,
                                    _pmdEDUCB* cb ) ;

      virtual INT32 onDeleteRecord( _dmsMBContext *context,
                                    const BSONObj &object,
                                    const dmsRecordID &rid,
                                    const _dmsRecordRW *pRecordRW,
                                    BOOLEAN markDeleting,
                                    _pmdEDUCB* cb ) ;

      virtual INT32 onUpdateRecord( _dmsMBContext *context,
                                    const BSONObj &orignalObj,
                                    const BSONObj &newObj,
                                    const dmsRecordID &rid,
                                    const _dmsRecordRW *pRecordRW,
                                    _pmdEDUCB* cb ) ;

      virtual INT32 onInsertIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   BOOLEAN isUnique,
                                   BOOLEAN isEnforce,
                                   const BSONObjSet &keySet,
                                   const dmsRecordID &rid,
                                   _pmdEDUCB* cb,
                                   utilWriteResult *pResult ) ;

      virtual INT32 onInsertIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   BOOLEAN isUnique,
                                   BOOLEAN isEnforce,
                                   const BSONObj &keyObj,
                                   const dmsRecordID &rid,
                                   _pmdEDUCB* cb,
                                   utilWriteResult *pResult ) ;

      virtual INT32 onUpdateIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   BOOLEAN isUnique,
                                   BOOLEAN isEnforce,
                                   const BSONObjSet &oldKeySet,
                                   const BSONObjSet &newKeySet,
                                   const dmsRecordID &rid,
                                   BOOLEAN isRollback,
                                   _pmdEDUCB* cb,
                                   utilWriteResult *pResult ) ;

      virtual INT32 onDeleteIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   BOOLEAN isUnique,
                                   const BSONObjSet &keySet,
                                   const dmsRecordID &rid,
                                   _pmdEDUCB* cb ) ;

   protected:
      enum _INSERT_CURSOR
      {
         _INSERT_NONE,
         _INSERT_CHECK
      } ;
      INT32         _checkInsertIndex( preIdxTreePtr &treePtr,
                                       _INSERT_CURSOR &insertCursor,
                                       const ixmIndexCB *indexCB,
                                       BOOLEAN isUnique,
                                       BOOLEAN isEnforce,
                                       const BSONObj &keyObj,
                                       const dmsRecordID &rid,
                                       _pmdEDUCB* cb,
                                       BOOLEAN allowSelfDup,
                                       utilWriteResult *pResult ) ;

   enum _DELETE_CURSOR
   {
      _DELETE_NONE,
      _DELETE_IGNORE,
      _DELETE_SAVE
   } ;
   INT32            _checkDeleteIndex( preIdxTreePtr &treePtr,
                                       _DELETE_CURSOR &deleteCursor,
                                       const ixmIndexCB *indexCB,
                                       BOOLEAN isUnique,
                                       const BSONObj &keyObj,
                                       const dmsRecordID &rid,
                                       _pmdEDUCB* cb ) ;

      INT32 _checkIDIndexUpdate( const dmsRecordID &rid,
                                 const BSONElement &idEle,
                                 _pmdEDUCB *cb ) ;

   private:

      INT32    saveOldVersionRecord( const _dmsRecordRW *pRecordRW,
                                     const dmsRecordID &rid,
                                     const BSONObj &obj,
                                     UINT32 ownnerTID ) ;

   private:
      dpsTransCB           *_transCB ;    // use it to access global old copy tree
      pmdEDUCB             *_eduCB ;

      // DMS related information
      _dmsRecordRW         * _recordRW ;
      // working area to be setup by callback function so the update can
      // put proper old copy into the area right before the update
      oldVersionContainer  *_oldVer ;

      /// control var
      BOOLEAN              _skipRecord ;
      INT32                _result ;
      BOOLEAN              _useOldVersion ;
      dpsOldRecordPtr      _recordPtr ;

      /// status var
      UINT32               _csLID ;
      UINT32               _clLID ;
      INT32                _csID ;
      UINT16               _clID ;
      SINT32               _latchedIdxLid ; // which we are holding a latch on
      INT32                _latchedIdxMode ;
      _rtnIXScanner       *_pScanner ;
      oldVersionUnitPtr    _unitPtr ;

      dmsTransRecordInfo   _recordInfo ;

   } ;

}

#endif // DMS_TRANS_LOCK_CALLBACK_HPP__

