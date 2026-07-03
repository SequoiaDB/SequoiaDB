/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

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
         ossMemset( this, DMS_LOB_INVALID_PAGEID, sizeof( _dmsBucketsManagementExtent ) ) ;
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
      _dmsStorageLob( const CHAR *lobmFileName,
                      const CHAR *lobdFileName,
                      dmsStorageInfo *info,
                      dmsStorageDataCommon *pDataSu,
                      utilCacheUnit* pCacheUnit ) ;
      virtual ~_dmsStorageLob() ;

      virtual void  syncMemToMmap( BOOLEAN *pHasWritten = NULL ) ;

      _dmsStorageLobData* getLobData() { return &_data ; }
      utilCacheUnit* getCacheUnit() { return _pCacheUnit ; }

      /// lobm file version, cached from header ( like _pageSize ), used to
      /// select the lob bucket hash algorithm without touching the mmap header
      OSS_INLINE UINT32 hashVersion() const { return _hashVersion ; }
      OSS_INLINE UTIL_LOB_HASH_TYPE hashType() const
      {
         return ( _hashVersion >= DMS_LOB_VERSION_3 ) ?
                UTIL_LOB_HASH_MD5 : UTIL_LOB_HASH_DJB2 ;
      }

      static UINT32 getBucketID( const _dmsLobDataMapBlk &blk,
                                 UTIL_LOB_HASH_TYPE hashType ) ;

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

      virtual void incWritePtrCount( INT32 collectionID ) ;
      virtual void decWritePtrCount( INT32 collectionID ) ;

      virtual DMS_FILE_TYPE getFileType() const { return DMS_FILE_LOB ; }

      INT32 rebuildBME() ;

   public:
      /// get the segment pages of lobd
      OSS_INLINE UINT32 dataSegmentPages() const ;

      INT32 freeCache( UINT32 segmentID ) ;

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
      INT32 _updateWithPage( const dmsLobRecord &record,
                             DMS_LOB_PAGEID pageID,
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
      INT32 _writeWithPage( const dmsLobRecord &record,
                            DMS_LOB_PAGEID &pageID,
                            const CHAR *pFullName,
                            dmsMBContext *mbContext,
                            BOOLEAN canUnLock,
                            _pmdEDUCB *cb,
                            dpsMergeInfo &info,
                            SDB_DPSCB *dpscb ) ;

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
      virtual INT32  _onMapMeta( UINT64 curOffSet, BOOLEAN isCreateNew ) ;
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
      OSS_INLINE static UINT32 _getBucket( UINT32 hash )
      {
         return hash & DMS_BUCKETS_MODULO ;
      }

      OSS_INLINE ossSpinSLatch* _getBucketLatch( UINT32 bucketID )
      {
         return &_vecBucketLacth[ bucketID % DMS_BUCKETS_LATCH_SIZE ] ;
      }

      /// calculate the hash of a blk using this file's hash algorithm
      OSS_INLINE UINT32 _calcHash( const _dmsLobDataMapBlk &blk ) const
      {
         return utilLobHash( blk._oid, blk._sequence, hashType() ) ;
      }

      INT32 _push2Bucket( UINT32 bucket, DMS_LOB_PAGEID pageId,
                          pmdEDUCB *cb,
                          _dmsLobDataMapBlk &blk,
                          const dmsLobRecord *pRecord = NULL ) ;

      INT32 _find( const _dmsLobRecord &record,
                   UINT32 clID,
                   pmdEDUCB *cb,
                   DMS_LOB_PAGEID &page,
                   UINT32 *bucket = NULL ) ;

      INT32 _allocatePage( const dmsLobRecord &record,
                           dmsMBContext *mbContext,
                           DMS_LOB_PAGEID &page ) ;

      INT32 _fillPage( const dmsLobRecord &record,
                       DMS_LOB_PAGEID page,
                       pmdEDUCB *cb,
                       dmsMBContext *mbContext ) ;

      /// only release space of page. will not change other meta data.
      INT32 _releasePage( DMS_LOB_PAGEID page, dmsMBContext *mbContext ) ;

      /// release space of page and change other meta data.
      INT32 _removePage( DMS_LOB_PAGEID page,
                         _dmsLobDataMapBlk *blk,
                         const UINT32 *bucket,
                         pmdEDUCB *cb,
                         dmsMBContext *mbContext,
                         BOOLEAN hasLockBucket,
                         BOOLEAN needRelease = TRUE,
                         const dmsLobRecord *pRecord = NULL ) ;

      INT32 _rollback( const dmsLobRecord &record,
                       DMS_LOB_PAGEID page,
                       pmdEDUCB *cb,
                       dmsMBContext *mbContext,
                       BOOLEAN pageFilled ) ;

      INT32 _renameMetaOrDataFile( const CHAR* metaFilePath,
                                   const CHAR* dataFilePath ) ;

      INT32 _checkIfMetaOrDataFileExist( const CHAR* metaFilePath,
                                         const CHAR* dataFilePath,
                                         BOOLEAN &exist ) ;

      void  _releasePath() ;

   private:
      dmsBucketsManagementExtent    *_dmsBME ;
      _dmsStorageData               *_dmsData ;
      _dmsStorageLobData            _data ;
      CHAR                         *_path ;
      CHAR                         *_metaPath ;
      BOOLEAN                       _needDelayOpen ;
      monSpinXLatch                 _delayOpenLatch ;

      utilCacheUnit                 *_pCacheUnit ;
      IDataSyncManager              *_pSyncMgrTmp ;
      IDataStatManager              *_pStatMgrTmp ;

      ossSpinSLatch                 *_vecBucketLacth ;

      BOOLEAN                       _isRename ;
      UINT32                        _dataSegmentSize ;
      /// cache of header _version, not use header ( like _pageSize )
      UINT32                        _hashVersion ;
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

