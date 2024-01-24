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

   Source File Name = dmsStorageIndex.hpp

   Descriptive Name = Data Management Service Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/08/2013  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSSTORAGE_INDEX_HPP_
#define DMSSTORAGE_INDEX_HPP_

#include "dmsStorageBase.hpp"
#include "dpsLogWrapper.hpp"
#include "dpsOp2Record.hpp"
#include "dmsPageMap.hpp"
#include "ossTypes.h"
#include "utilResult.hpp"
#include "utilList.hpp"
#include "dmsOprHandler.hpp"
#include "dmsTaskStatus.hpp"
#include "dmsWriteGuard.hpp"

using namespace bson ;

namespace engine
{

   class _dmsStorageDataCommon ;
   class _dmsStorageData ;
   class _pmdEDUCB ;
   class _ixmIndexCB ;
   class _dmsMBContext ;
   class _ixmKey ;
   class _dmsDupKeyProcessor ;

   #define DMS_INDEXSU_EYECATCHER         "SDBIDX"
   #define DMS_INDEXSU_CUR_VERSION        1

   class _dmsExtraRecord : public SDBObject
   {
   public:
      _dmsExtraRecord( const CHAR *clName, BSONObj record, BOOLEAN isInsert )
      {
         _clName = clName ;
         _record = record ;
         _isInsert = isInsert ;
      }

      _dmsExtraRecord()
      {
         _isInsert = TRUE ;
      }

      ~_dmsExtraRecord()
      {
      }

   public:
      ossPoolString _clName ;
      BSONObj _record ;
      BOOLEAN _isInsert ;
   } ;

   typedef _utilList<_dmsExtraRecord> DMS_RECORD_VEC ;
   class _dmsRecordContainer : public SDBObject
   {
   public:
      _dmsRecordContainer()
      {
      }

      ~_dmsRecordContainer()
      {
         _recordVec.clear() ;
      }

   public:
      INT32 append( const CHAR *clName, const BSONObj &record,
                    BOOLEAN isInsert = TRUE )
      {
         INT32 rc = SDB_OK ;
         try
         {
            _dmsExtraRecord exRecord( clName, record.getOwned(), isInsert ) ;
            rc = _recordVec.push_back( exRecord ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to push back record, rc: %d",
                         rc ) ;
         }
         catch ( std::exception &e )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to append record, exception: %s, rc: %d",
                    e.what(), rc ) ;
            goto error ;
         }

      done:
         return rc ;
      error:
         goto done ;
      }

      UINT32 getSize()
      {
         return _recordVec.size() ;
      }

   public:
      DMS_RECORD_VEC _recordVec ;
   } ;

   /*
      _dmsStorageIndex define
   */
   class _dmsStorageIndex : public _dmsStorageBase
   {
      public:
         _dmsStorageIndex ( IStorageService *service,
                            dmsSUDescriptor *suDescriptor,
                            const CHAR *pSuFileName,
                            _dmsStorageDataCommon *pDataSu ) ;
         ~_dmsStorageIndex () ;

         virtual void  syncMemToMmap() ;

         dmsPageMapUnit*   getPageMapUnit() ;
         dmsPageMap*       getPageMap( UINT16 mbID ) ;

      public:
         // reserve a signal page
         INT32    reserveExtent ( UINT16 mbID, dmsExtentID &extentID,
                                  _dmsContext *context ) ;
         // release a signal page
         INT32    releaseExtent ( dmsExtentID extentID,
                                  BOOLEAN setFlag = FALSE ) ;

         INT32    createIndex ( _dmsMBContext *context, const BSONObj &index,
                                _pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                BOOLEAN isSys = FALSE,
                                INT32 sortBufferSize = SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE,
                                utilWriteResult *pResult = NULL,
                                dmsIdxTaskStatus* pIdxStatus = NULL,
                                BOOLEAN forceTransCallback = FALSE,
                                BOOLEAN addUIDIfNotExist = TRUE ) ;

         INT32    dropIndex ( _dmsMBContext *context, OID &indexOID,
                              _pmdEDUCB *cb, SDB_DPSCB *dpscb,
                              BOOLEAN isSys = FALSE,
                              dmsIdxTaskStatus *pIdxStatus = NULL,
                              BOOLEAN onlyStandalone = FALSE ) ;
         INT32    dropIndex( _dmsMBContext *context, const CHAR *indexName,
                             _pmdEDUCB *cb, SDB_DPSCB *dpscb,
                             BOOLEAN isSys = FALSE,
                             dmsIdxTaskStatus *pIdxStatus = NULL,
                             BOOLEAN onlyStandalone = FALSE ) ;
         INT32    dropIndex( _dmsMBContext *context, INT32 indexID,
                             dmsExtentID indexLID, _pmdEDUCB *cb,
                             SDB_DPSCB *dpscb, BOOLEAN isSys = FALSE,
                             dmsIdxTaskStatus *pIdxStatus = NULL ) ;

         INT32    dropAllIndexes( _dmsMBContext *context, _pmdEDUCB *cb,
                                  SDB_DPSCB *dpscb ) ;

         INT32    rebuildIndexes ( _dmsMBContext *context, _pmdEDUCB *cb,
                                   INT32 sortBufferSize = SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE,
                                   _dmsDupKeyProcessor *dkProcessor = NULL ) ;
         INT32    rebuildIndex( _dmsMBContext *context,
                                dmsExtentID indexExtID,
                                ixmIndexCB &indexCB,
                                _pmdEDUCB *cb,
                                INT32 sortBufferSize = SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE,
                                _dmsDupKeyProcessor *dkProcessor = NULL ) ;

         // Caller must hold mb exclusive lock
         INT32    indexesInsert ( _dmsMBContext *context, dmsExtentID extLID,
                                  BSONObj &inputObj, const dmsRecordID &rid,
                                  _pmdEDUCB *cb,
                                  IDmsOprHandler *pOprHandle,
                                  dmsWriteGuard &writeGuard,
                                  utilWriteResult *pResult = NULL,
                                  dpsUnqIdxHashArray *pUnqIdxHashArray = NULL ) ;

         // Caller must hold mb exclusive lock
         INT32    indexesUpdate ( _dmsMBContext *context, dmsExtentID extLID,
                                  BSONObj &originalObj, BSONObj &newObj,
                                  const dmsRecordID &rid,
                                  _pmdEDUCB *cb,
                                  BOOLEAN isUndo,
                                  IDmsOprHandler *pOprHandle,
                                  dmsWriteGuard &writeGuard,
                                  const ixmIdxHashBitmap &idxHashBitmap,
                                  utilWriteResult *pResult = NULL,
                                  dpsUnqIdxHashArray *pNewUnqIdxHashArray = NULL,
                                  dpsUnqIdxHashArray *pOldUnqIdxHashArray = NULL ) ;

         // Caller must hold mb exclusive lock
         INT32    indexesDelete ( _dmsMBContext *context, dmsExtentID extLID,
                                  BSONObj &inputObj, const dmsRecordID &rid,
                                  _pmdEDUCB *cb,
                                  IDmsOprHandler *pOprHandle,
                                  dmsWriteGuard &writeGuard,
                                  BOOLEAN isUndo = FALSE,
                                  dpsUnqIdxHashArray *pUnqIdxHashArray = NULL ) ;

         INT32    truncateIndexes ( _dmsMBContext *context,
                                    _pmdEDUCB *cb ) ;

         INT32    getIndexCBExtent ( _dmsMBContext *context,
                                     const CHAR *indexName,
                                     dmsExtentID &indexExtent ) ;

         INT32    getIndexCBExtent ( _dmsMBContext *context,
                                     const OID &indexOID,
                                     dmsExtentID &indexExtent ) ;

         INT32    getIndexCBExtent ( _dmsMBContext *context,
                                     INT32 indexID,
                                     dmsExtentID &indexExtent ) ;

         INT32    checkIndexCBExtentExist( _dmsMBContext *context,
                                           dmsExtentID indexExtent,
                                           BOOLEAN &exist ) ;

         void     addStatFreeSpace ( UINT16 mbID, UINT16 size ) ;
         void     decStatFreeSpace ( UINT16 mbID, UINT16 size ) ;

         INT32    indexKeySizeMax() { return _idxKeySizeMax ; }

         INT32    getIndex( _dmsMBContext *context,
                            _ixmIndexCB *indexCB,
                            pmdEDUCB *cb,
                            std::shared_ptr<IIndex> &idxPtr ) ;
         INT32    checkProcess( _dmsMBContext *context,
                                const dmsRecordID &rid,
                                dmsIndexWriteGuard &writeGuard ) ;

      private:
         INT32    _releaseMetaExtent( dmsExtentID extentID ) ;

         INT32    _createIndex( _dmsMBContext *context,
                                const BSONObj &index,
                                dmsExtentID metaExtentID,
                                dmsExtentID rootExtentID,
                                UINT16 indexType,
                                _pmdEDUCB *cb,
                                SDB_DPSCB *dpscb,
                                BOOLEAN isSys,
                                INT32 sortBufferSize,
                                utilWriteResult *pResult,
                                dmsIdxTaskStatus *pIdxStatus,
                                BOOLEAN forceTransCallback,
                                BOOLEAN addUIDIfNotExist ) ;

         INT32    _checkForCrtTextIdx( _dmsMBContext *context,
                                       const BSONObj &index ) ;

         // newIndex - 'ExtDataName' will be added into index.
         INT32    _createTextIdx( _dmsMBContext *context,
                                  const BSONObj &index,
                                  dmsExtentID metaExtentID,
                                  dmsExtentID rootExtentID,
                                  _pmdEDUCB *cb,
                                  SDB_DPSCB *dpscb,
                                  dmsIdxTaskStatus* pIdxStatus,
                                  BOOLEAN addUIDIfNotExist ) ;

         // if indexLID == DMS_INALID_EXTENT, it will get from index cb
         INT32    _rebuildIndex( _dmsMBContext *context,
                                 dmsExtentID indexExtentID,
                                 dmsExtentID indexLID, _pmdEDUCB *cb,
                                 INT32 sortBufferSize,
                                 UINT16 indexType,
                                 dmsIndexBuildGuardPtr &guardPtr,
                                 IDmsOprHandler *pOprHandle = NULL,
                                 utilWriteResult *pResult = NULL,
                                 _dmsDupKeyProcessor *dkProcessor = NULL,
                                 dmsIdxTaskStatus* pIdxStatus = NULL ) ;

         INT32    _indexInsert ( _dmsMBContext *context, _ixmIndexCB *indexCB,
                                 BSONObj &inputObj, const dmsRecordID &rid,
                                 _pmdEDUCB *cb, BOOLEAN dupAllowed,
                                 BOOLEAN dropDups,
                                 IDmsOprHandler *pOprHandle,
                                 dmsWriteGuard &writeGuard,
                                 utilWriteResult *pResult = NULL,
                                 dpsUnqIdxHashArray *pUnqIdxHashArray = NULL ) ;

         INT32    _indexUpdate ( _dmsMBContext *context, _ixmIndexCB *indexCB,
                                 BSONObj &originalObj, BSONObj &newObj,
                                 const dmsRecordID &rid, _pmdEDUCB *cb,
                                 BOOLEAN isRollback,
                                 IDmsOprHandler *pOprHandle,
                                 dmsWriteGuard &writeGuard,
                                 utilWriteResult *pResult = NULL,
                                 dpsUnqIdxHashArray *pUnqIdxHashArray = NULL,
                                 dpsUnqIdxHashArray *pOldUnqIdxHashArray = NULL ) ;

         INT32    _indexDelete ( _dmsMBContext *context, _ixmIndexCB *indexCB,
                                 BSONObj &inputObj, const dmsRecordID &rid,
                                 _pmdEDUCB *cb,
                                 IDmsOprHandler *pOprHandle,
                                 dmsWriteGuard &writeGuard,
                                 dpsUnqIdxHashArray *pUnqIdxHashArray = NULL ) ;

         INT32    _indexInsert( dmsMBContext *context,
                                _ixmIndexCB *indexCB,
                                const bson::BSONObj &key,
                                const dmsRecordID &rid,
                                _pmdEDUCB *cb,
                                BOOLEAN allowDuplicated,
                                utilWriteResult *pResult = NULL ) ;
         INT32    _indexDelete( _dmsMBContext *context,
                                _ixmIndexCB *indexCB,
                                const bson::BSONObj &key,
                                const dmsRecordID &rid,
                                dmsWriteGuard &writeGuard,
                                _pmdEDUCB *cb ) ;

         INT32    _builderIndexRecord( ixmIndexCB *indexCB, const _ixmKey &key,
                                       BSONObj &record ) ;

         INT32    _needProcessIndex( dmsMBContext *context,
                                     ixmIndexCB &indexCB,
                                     UINT32 indexID,
                                     const dmsRecordID &rid,
                                     dmsIndexWriteGuard &writeGuard,
                                     BOOLEAN &needProcess ) ;

         INT32    _collectGIDXRecord( ixmIndexCB &indexCB, const _ixmKey &key,
                                      BOOLEAN isInsert,
                                      _dmsRecordContainer &container ) ;

         INT32    _submitGIDXRecords( _dmsMBContext *context,
                                      _dmsRecordContainer &container,
                                      _pmdEDUCB *cb ) ;

         INT32    _globalIndexesDelete( _dmsMBContext *context,
                                        const dmsRecordID &rid,
                                        BSONObj &inputObj,
                                        dmsIndexWriteGuard &writeGuard,
                                        _pmdEDUCB *cb ) ;

         INT32    _globalIndexesUpdate( _dmsMBContext *context,
                                        const dmsRecordID &rid,
                                        BSONObj &originalObj,
                                        BSONObj &newObj,
                                        dmsIndexWriteGuard &writeGuard,
                                        _pmdEDUCB *cb,
                                        BOOLEAN isRollback,
                                        const ixmIdxHashBitmap &idxHashBitmap,
                                        utilWriteResult *pResult = NULL ) ;

         INT32    _globalIndexesInsert( _dmsMBContext *context,
                                        const dmsRecordID &rid,
                                        BSONObj &inputObj,
                                        dmsIndexWriteGuard &writeGuard,
                                        _pmdEDUCB *cb,
                                        utilWriteResult *pResult = NULL ) ;

         BOOLEAN  _needProcessGlobalIndex( _dmsMBContext *context,
                                           _pmdEDUCB *cb ) ;
         BOOLEAN  _needUpdateIndexes( _dmsMBContext *context,
                                      const ixmIdxHashBitmap &idxHashBitmap ) ;

         INT32    _buildIndexUniqueID( utilIdxUniqueID& uniqID ) ;

         INT32    _checkAndChangeUniqueID( _dmsMBContext *context,
                                           INT32 indexID,
                                           const BSONObj &index ,
                                           pmdEDUCB *cb,
                                           SDB_DPSCB *dpscb,
                                           dmsIdxTaskStatus* pIdxStatus ) ;

      private:
         virtual UINT64 _dataOffset() ;
         virtual const CHAR* _getEyeCatcher() const ;
         virtual UINT32 _curVersion() const ;
         virtual INT32  _checkVersion( dmsStorageUnitHeader *pHeader ) ;
         virtual INT32  _onCreate( OSSFILE *file, UINT64 curOffSet ) ;
         virtual INT32  _onMapMeta( UINT64 curOffSet ) ;
         virtual INT32  _onOpened() ;
         virtual void   _onClosed() ;

         virtual INT32  _onFlushDirty( BOOLEAN force, BOOLEAN sync ) ;

         virtual INT32  _onMarkHeaderValid( UINT64 &lastLSN,
                                            BOOLEAN sync,
                                            UINT64 lastTime,
                                            BOOLEAN &setHeadCommFlgValid ) ;

         virtual INT32  _onMarkHeaderInvalid( INT32 collectionID ) ;

         virtual UINT64 _getOldestWriteTick() const ;

         virtual void   _onRestore() ;

         virtual BOOLEAN _canRecreateNew() ;

         INT32 _allocateIdxID( _dmsMBContext *context,
                               const CHAR *indexName,
                               const BSONObj &index,
                               INT32 &indexID ) ;

         INT32 _registerBuildGuard( const dmsIdxMetadataKey &key,
                                    const dmsRecordID rid,
                                    dmsIndexBuildGuardPtr &guardPtr ) ;
         void _unregisterBuildGuard( const dmsIdxMetadataKey &key ) ;
         dmsIndexBuildGuardPtr _getBuildGuard( const dmsIdxMetadataKey &key ) ;

      private:
         _dmsStorageData         *_pDataSu ;
         dmsPageMapUnit          _mbPageInfo ;
         INT32                   _idxKeySizeMax ; // max size of index key value

         // locks to protect index build
         ossRWMutex _buildGuardsMutex ;
         dmsIdxBuildGuardMap _buildGuards ;

      friend class _dmsIndexBuilder ;
   };
   typedef _dmsStorageIndex dmsStorageIndex ;
}

#endif //DMSSTORAGE_INDEX_HPP_

