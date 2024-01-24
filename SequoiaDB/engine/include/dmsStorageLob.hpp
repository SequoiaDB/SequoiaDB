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

   Source File Name = dmsStorageLob.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          17/07/2014  YW Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_STORAGELOB_HPP_
#define DMS_STORAGELOB_HPP_

#include "dmsLobDef.hpp"
#include "dmsStorageLobData.hpp"
#include "dmsStorageData.hpp"
#include "monLatch.hpp"
#include "utilCache.hpp"

namespace engine
{
   #define DMS_BME_OFFSET        DMS_MME_OFFSET
   #define DMS_LOBM_EYECATCHER               "SDBLOBM"
   #define DMS_LOBM_EYECATCHER_LEN           8

   /// must be power of 2
   const UINT32 DMS_BUCKETS_NUM =   16777216 ; // 16MB
   #define DMS_BUCKETS_MODULO       16777215
   #define DMS_BUCKETS_LATCH_SIZE   128

   /// record is dmsLobRecord
   #define DMS_IS_LOBMETA_RECORD(record) \
             (DMS_LOB_META_SEQUENCE == record._sequence && 0 == record._offset)
   /// len is dmsLobDataMapBlk._dataLen
   #define DMS_GET_LOB_PIECE_LENGTH(len) \
             (DMS_LOB_META_LENGTH >= len ? 0 : (len - DMS_LOB_META_LENGTH))

   /*
      _dmsBucketsManagementExtent define
   */
   struct _dmsBucketsManagementExtent : public SDBObject
   {
      INT32 _buckets[DMS_BUCKETS_NUM] ;

      _dmsBucketsManagementExtent()
      {
         ossMemset( this, 0xff,
                    sizeof( _dmsBucketsManagementExtent ) ) ;
      }
   } ;
   typedef struct _dmsBucketsManagementExtent dmsBucketsManagementExtent ;

   #define DMS_BME_SZ (sizeof( dmsBucketsManagementExtent ))


   class _pmdEDUCB ;

   /*
      _dmsStorageLob define
   */
   class _dmsStorageLob : public _dmsStorageBase
   {
   public:
      _dmsStorageLob( IStorageService *service,
                      dmsSUDescriptor *suDescriptor,
                      const CHAR *lobmFileName,
                      const CHAR *lobdFileName,
                      dmsStorageDataCommon *pDataSu,
                      utilCacheUnit* pCacheUnit ) ;
      virtual ~_dmsStorageLob() ;

      virtual void  syncMemToMmap() ;

      _dmsStorageLobData* getLobData() { return &_data ; }
      utilCacheUnit* getCacheUnit() { return _pCacheUnit ; }

   public:
      INT32 open( const CHAR *path,
                  const CHAR *metaPath,
                  IDataSyncManager *pSyncMgr,
                  IDataStatManager *pStatMgr,
                  BOOLEAN createNew ) ;

      void  removeStorageFiles() ;

      INT32 rename( const CHAR *csName,
                    const CHAR *lobmFileName,
                    const CHAR *lobdFileName ) ;

      INT32 getLobMeta( const bson::OID &oid,
                        dmsMBContext *mbContext,
                        _pmdEDUCB *cb,
                        _dmsLobMeta &meta ) ;

      INT32 writeLobMeta( const bson::OID &oid,
                          dmsMBContext *mbContext,
                          _pmdEDUCB *cb,
                          const _dmsLobMeta &meta,
                          BOOLEAN isNew,
                          SDB_DPSCB *dpsCB ) ;

      INT32 write( const dmsLobRecord &record,
                   dmsMBContext *mbContext,
                   _pmdEDUCB *cb,
                   SDB_DPSCB *dpscb ) ;

      INT32 update( const dmsLobRecord &record,
                    dmsMBContext *mbContext,
                    _pmdEDUCB *cb,
                    SDB_DPSCB *dpscb ) ;

      INT32 writeOrUpdate( const dmsLobRecord &record,
                           dmsMBContext *mbContext,
                           _pmdEDUCB *cb,
                           SDB_DPSCB *dpscb,
                           BOOLEAN *pHasUpdated = NULL ) ;

      INT32 remove( const dmsLobRecord &record,
                    dmsMBContext *mbContext,
                    _pmdEDUCB *cb,
                    SDB_DPSCB *dpscb,
                    BOOLEAN onlyRemoveNewPage = FALSE,
                    const CHAR *pOldData = NULL ) ;

      /// user should make sure that the length of
      ///  buf is enough
      INT32 read( const dmsLobRecord &record,
                  dmsMBContext *mbContext,
                  _pmdEDUCB *cb,
                  CHAR *buf,
                  UINT32 &len ) ;

      INT32 readPage( DMS_LOB_PAGEID &pos,
                      BOOLEAN onlyMetaPage,
                      _pmdEDUCB *cb,
                      dmsMBContext *mbContext,
                      dmsLobInfoOnPage &page ) ;

      INT32 truncate( dmsMBContext *mbContext,
                      _pmdEDUCB *cb,
                      SDB_DPSCB *dpscb ) ;

      virtual BOOLEAN isOpened() const { return _data.isOpened() ; }

      INT32 rebuildBME() ;

   public:
      /// get the segment pages of lobd
     OSS_INLINE UINT32 dataSegmentPages() const ;

   protected:
      INT32  _openLob( const CHAR *path,
                       const CHAR *metaPath,
                       BOOLEAN createNew ) ;
      INT32 _delayOpen() ;

      INT32 _calcCount() ;

      OSS_INLINE const CHAR*   _clFullName ( const CHAR *clName,
                                             CHAR *clFullName,
                                             UINT32 fullNameLen ) ;

      /*
         Caller must hold lock with EXCLUSIVE
      */
      INT32 _updateWithDpslog( const dmsLobRecord &record,
                               const CHAR *pFullName,
                               dmsMBContext *mbContext,
                               BOOLEAN canUnLock,
                               _pmdEDUCB *cb,
                               SDB_DPSCB *dpscb ) ;

      /*
         1. Caller must hold lock with EXCLUSIVE
         2. pageID is taken by _writeWithPage no matter
            whether it succeeds or not
      */
      INT32 _writeWithDpslog( const dmsLobRecord &record,
                              const CHAR *pFullName,
                              dmsMBContext *mbContext,
                              BOOLEAN canUnLock,
                              BOOLEAN updateWhenExist,
                              _pmdEDUCB *cb,
                              dpsMergeInfo &info,
                              SDB_DPSCB *dpscb,
                              BOOLEAN *hasUpdated ) ;

      INT32 _writeInner( const dmsLobRecord &record,
                         dmsMBContext *mbContext,
                         _pmdEDUCB *cb,
                         SDB_DPSCB *dpscb,
                         BOOLEAN updateWhenExist,
                         BOOLEAN *pHasUpdated ) ;


      void _statVaildLobSize( dmsMBContext *mbContext,
                              const dmsLobMeta *metaNew,
                              const dmsLobMeta *metaOld ) ;

   private:
      virtual INT32  _onCreate( OSSFILE *file, UINT64 curOffSet ) ;
      virtual INT32  _onMapMeta( UINT64 curOffSet ) ;
      virtual UINT32 _getSegmentSize() const ;
      virtual UINT32 _getMetaSizeOfDataSegment() const ;
      virtual UINT32 _getDataSegmentPages() const ;
      virtual UINT32 _extendThreshold() const ;
      virtual UINT64 _dataOffset() ;
      virtual INT32  _extendSegments( UINT32 numSeg ) ;
      virtual const CHAR* _getEyeCatcher() const ;
      virtual UINT32 _curVersion() const ;
      virtual INT32  _checkVersion( dmsStorageUnitHeader *pHeader ) ;
      virtual void   _onClosed() ;
      virtual INT32  _onOpened() ;
      virtual void   _initHeaderPageSize( dmsStorageUnitHeader *pHeader,
                                          dmsStorageInfo *pInfo ) ;
      virtual INT32  _checkPageSize( dmsStorageUnitHeader *pHeader ) ;

      virtual BOOLEAN _checkFileSizeValidBySegment( const UINT64 fileSize,
                                                    UINT64 &rightSize ) ;

      virtual BOOLEAN _checkFileSizeValid( const UINT64 fileSize,
                                           UINT64 &rightSize ) ;

      virtual void    _calcExtendInfo( const UINT64 fileSize,
                                       UINT32 &numSeg,
                                       UINT64 &incFileSize,
                                       UINT32 &incPageNum ) ;

      /// flush callback:  SDB_OK: continue, no SDB_OK: stop
      virtual INT32  _onFlushDirty( BOOLEAN force, BOOLEAN sync ) ;

      virtual INT32  _onMarkHeaderValid( UINT64 &lastLSN,
                                         BOOLEAN sync,
                                         UINT64 lastTime,
                                         BOOLEAN &setHeadCommFlgValid ) ;

      virtual INT32  _onMarkHeaderInvalid( INT32 collectionID ) ;

      virtual UINT64 _getOldestWriteTick() const ;

      virtual void   _onRestore() ;

   private:
      OSS_INLINE UINT32 _getBucket( UINT32 hash )
      {
         return hash & DMS_BUCKETS_MODULO ;
      }

      OSS_INLINE ossSpinSLatch* _getBucketLatch( UINT32 bucketID )
      {
         return _vecBucketLacth[ bucketID % DMS_BUCKETS_LATCH_SIZE ] ;
      }

      INT32 _push2Bucket( UINT32 bucket, DMS_LOB_PAGEID pageId,
                          pmdEDUCB *cb,
                          _dmsLobDataMapBlk &blk,
                          const dmsLobRecord *pRecord = NULL ) ;

      INT32 _renameMetaOrDataFile( const CHAR* metaFilePath,
                                   const CHAR* dataFilePath ) ;

      INT32 _checkIfMetaOrDataFileExist( const CHAR* metaFilePath,
                                         const CHAR* dataFilePath,
                                         BOOLEAN &exist ) ;
   private:
      dmsBucketsManagementExtent    *_dmsBME ;
      _dmsStorageData               *_dmsData ;
      _dmsStorageLobData            _data ;
      CHAR                          _path[ OSS_MAX_PATHSIZE + 1 ] ;
      CHAR                          _metaPath[ OSS_MAX_PATHSIZE + 1 ] ;
      BOOLEAN                       _needDelayOpen ;
      monSpinXLatch                 _delayOpenLatch ;

      utilCacheUnit                 *_pCacheUnit ;
      IDataSyncManager              *_pSyncMgrTmp ;
      IDataStatManager              *_pStatMgrTmp ;

      vector< ossSpinSLatch* >      _vecBucketLacth ;

      BOOLEAN                       _isRename ;
      UINT32                        _dataSegmentSize ;
   } ;
   typedef class _dmsStorageLob dmsStorageLob ;

   /*
      _dmsStorageLob OSS_INLINE functions :
   */
   OSS_INLINE UINT32 _dmsStorageLob::dataSegmentPages() const
   {
      return _getDataSegmentPages() ;
   }

   OSS_INLINE const CHAR* _dmsStorageLob::_clFullName( const CHAR *clName,
                                                       CHAR * clFullName,
                                                       UINT32 fullNameLen )
   {
      SDB_ASSERT( fullNameLen > DMS_COLLECTION_FULL_NAME_SZ,
                  "Collection full name len error" ) ;
      ossStrncat( clFullName, getSuName(), DMS_COLLECTION_SPACE_NAME_SZ ) ;
      ossStrncat( clFullName, ".", 1 ) ;
      ossStrncat( clFullName, clName, DMS_COLLECTION_NAME_SZ ) ;
      clFullName[ DMS_COLLECTION_FULL_NAME_SZ ] = 0 ;

      return clFullName ;
   }

}

#endif // DMS_STORAGELOB_HPP_

