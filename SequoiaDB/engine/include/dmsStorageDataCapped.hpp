/*******************************************************************************


   Copyright (C) 2011-2016 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = dmsStorageDataCapped.hpp

   Descriptive Name = Data Management wrapper for data type.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/11/2016  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSSTORAGE_DATACAPPED_HPP
#define DMSSTORAGE_DATACAPPED_HPP

#include "dmsStorageDataCommon.hpp"

namespace engine
{
#define DMS_INVALID_REC_LOGICALID         -1

#define DMS_DFT_CAPPEDCL_SIZE             (30 * 1024 * 1024 * 1024LL)
#define DMS_DFT_CAPPEDCL_RECNUM           0

#define DMS_INVALID_LOGICALID             (-1)

#pragma pack(1)
   class _LogicalIDToInsert : public SDBObject
   {
   public :
      CHAR  _type ;
      CHAR  _id[4] ; // _id + '\0'
      INT64 _logicalID ;
      _LogicalIDToInsert ()
      {
         _type = (CHAR)NumberLong ;
         _id[0] = '_' ;
         _id[1] = 'i' ;
         _id[2] = 'd' ;
         _id[3] = 0 ;
         _logicalID = DMS_INVALID_LOGICALID ;
         SDB_ASSERT ( sizeof ( _LogicalIDToInsert) == 13,
                      "LogicalIDToInsert should be 13 bytes" ) ;
      }
   } ;
   typedef class _LogicalIDToInsert LogicalIDToInsert ;

   /*
      _idToInsert define
   */
   class _LogicalIDToInsertEle : public BSONElement
   {
   public :
      _LogicalIDToInsertEle( CHAR* x ) : BSONElement((CHAR*) ( x )){}
   } ;
   typedef class _LogicalIDToInsertEle LogicalIDToInsertEle ;
#pragma pack()

   class _pmdEDUCB ;
   class _mthModifier ;

   struct _dmsCappedCLOptions
   {
      INT64 _maxSize ;
      INT64 _maxRecNum ;
      BOOLEAN _overwrite ;

      _dmsCappedCLOptions()
      {
         _maxSize = DMS_DFT_CAPPEDCL_SIZE ;
         _maxRecNum = DMS_DFT_CAPPEDCL_RECNUM ;
         _overwrite = FALSE ;
      }
   } ;
   typedef _dmsCappedCLOptions dmsCappedCLOptions ;

   struct _dmsExtentInfo
   {
      dmsExtentID    _id ;
      INT64          _extLogicID ;
      UINT32         _recCount ;
      UINT32         _freeSpace ;
      dmsOffset      _firstRecordOffset ;
      dmsOffset      _lastRecordOffset ;
      UINT32         _writePos ; // Currently write position in the working
      UINT32         _recNo ;
      _dmsExtentInfo()
      {
         reset() ;
      }

      void reset()
      {
         _id = DMS_INVALID_EXTENT ;
         _extLogicID = DMS_INVALID_EXTENT ;
         _recCount = 0 ;
         _freeSpace = 0 ;
         _firstRecordOffset = DMS_INVALID_OFFSET ;
         _lastRecordOffset = DMS_INVALID_OFFSET ;
         _writePos = 0 ;
         _recNo = 0 ;
      }

      const dmsExtentID getID() const { return _id ; }

      dmsOffset getNextRecOffset() const
      {
         return _writePos + DMS_EXTENT_METADATA_SZ ;
      }

      INT64 getRecordLogicID()
      {
         return ( _extLogicID * DMS_CAP_EXTENT_BODY_SZ + _writePos ) ;
      }

      UINT32 currentRecNo() const
      {
         return _recNo ;
      }

      void recNoInc()
      {
         _recNo++ ;
      }

      void seek( dmsOffset offset )
      {
         SDB_ASSERT( offset >= (INT32)DMS_EXTENT_METADATA_SZ,
                     "offset is invalid" ) ;
         _writePos = offset - DMS_EXTENT_METADATA_SZ ;
         _freeSpace = DMS_CAP_EXTENT_SZ - offset ;
      }

      string toString() const
      {
         ostringstream ss ;
         ss << "Extent id: " << _id
            << ". Extent logical id: " << _extLogicID
            << ". Record number: " << _recCount
            << ". Free space: " << _freeSpace
            << ". First record offset: " << _firstRecordOffset
            << ". Last record offset: " << _lastRecordOffset
            << ". Write position: " << _writePos
            << ". Record number: " << _recNo
            << ".\n" ;
         return ss.str() ;
      }
   } ;
   typedef _dmsExtentInfo dmsExtentInfo ;

   class _dmsStorageDataCapped : public _dmsStorageDataCommon
   {
      typedef std::map<UINT32, UINT32>    SIZE_REQ_MAP ;
   public:
      _dmsStorageDataCapped( const CHAR* pSuFileName,
                             dmsStorageInfo *pInfo,
                             _IDmsEventHolder *pEventHolder ) ;
      virtual ~_dmsStorageDataCapped() ;

      virtual INT32 popRecord ( dmsMBContext *context,
                                INT64 logicalID,
                                _pmdEDUCB *cb,
                                SDB_DPSCB *dpscb,
                                INT8 direction = 1 ) ;

      virtual INT32 dumpExtOptions( dmsMBContext *context,
                                    BSONObj &extOptions ) ;

      OSS_INLINE dmsExtentInfo* getWorkExtInfo( UINT16 mbID ) ;

      virtual INT32 postDataRestored( dmsMBContext * context ) ;

   private:
      virtual const CHAR* _getEyeCatcher() const ;
      virtual INT32 _onOpened() ;
      virtual void _onClosed() ;
      virtual void _onRestore() ;
      virtual INT32 _onFlushDirty( BOOLEAN force, BOOLEAN sync ) ;

      virtual INT32 _onCollectionTruncated( dmsMBContext *context ) ;
      virtual INT32 _prepareAddCollection( const BSONObj *extOption,
                                           dmsExtentID &extOptExtent,
                                           UINT16 &extentPageNum ) ;

      virtual INT32 _onAddCollection( const BSONObj *extOption,
                                      dmsExtentID extOptExtent,
                                      UINT32 extentSize,
                                      UINT16 collectionID ) ;

      virtual void _onAllocExtent( dmsMBContext *context,
                                   dmsExtent * extAddr,
                                   SINT32 extentID ) ;

      virtual INT32 _prepareInsertData( const BSONObj &record,
                                        BOOLEAN mustOID,
                                        pmdEDUCB *cb,
                                        dmsRecordData &recordData,
                                        BOOLEAN &memReallocate,
                                        INT64 position ) ;

      virtual void _finalRecordSize( UINT32 &size,
                                     const dmsRecordData &recordData ) ;

      virtual INT32 _allocRecordSpace( dmsMBContext *context,
                                       UINT32 size,
                                       dmsRecordID &foundRID,
                                       _pmdEDUCB *cb ) ;

      virtual INT32 _allocRecordSpaceByPos( dmsMBContext *context,
                                            UINT32 size,
                                            INT64 position,
                                            dmsRecordID &foundRID,
                                            _pmdEDUCB *cb ) ;

      virtual INT32 _extentInsertRecord( dmsMBContext *context,
                                         dmsExtRW &extRW,
                                         dmsRecordRW &recordRW,
                                         const dmsRecordData &recordData,
                                         UINT32 recordSize,
                                         _pmdEDUCB *cb,
                                         BOOLEAN isInsert = TRUE ) ;

      virtual INT32 _extentUpdatedRecord( dmsMBContext *context,
                                          dmsExtRW &extRW,
                                          dmsRecordRW &recordRW,
                                          const dmsRecordData &recordData,
                                          const BSONObj &newObj,
                                          _pmdEDUCB *cb ) ;

      virtual INT32 _extentRemoveRecord( dmsMBContext *context,
                                         dmsExtRW &extRW,
                                         dmsRecordRW &recordRW,
                                         _pmdEDUCB *cb,
                                         BOOLEAN decCount = TRUE ) ;

      virtual INT32 _onInsertFail( dmsMBContext *context,
                                   BOOLEAN hasInsert,
                                   dmsRecordID rid,
                                   SDB_DPSCB *dpscb,
                                   ossValuePtr dataPtr,
                                   _pmdEDUCB *cb ) ;

      virtual INT32 extractData( dmsMBContext *mbContext,
                                 dmsRecordRW &recordRW,
                                 _pmdEDUCB *cb,
                                 dmsRecordData &recordData ) ;

      virtual INT32 _operationPermChk( DMS_ACCESS_TYPE accessType ) ;

      virtual void _onAllocSpaceReady( dmsContext *context, BOOLEAN &doit ) ;

      INT32 _parseExtendOptions( const BSONObj *extOptions,
                                 dmsCappedCLOptions &options ) ;

      INT32 _recycleOneExtent( dmsMBContext *context ) ;
      INT32 _recycleWorkExt( dmsMBContext *context, dmsExtentID extID ) ;
      INT32 _recycleActiveExt( dmsMBContext *context,
                               dmsExtentID extID ) ;

      INT32 _popFromActiveExt( dmsMBContext *context,
                               dmsExtentID extentID,
                               dmsOffset offset,
                               INT8 direction ) ;

      INT32 _syncWorkExtInfo( UINT16 collectionID ) ;
      INT32 _switchWorkExt( dmsMBContext *context, dmsExtentID extID ) ;
      INT32 _attachWorkExt( UINT16 collectionID, dmsExtentID extID ) ;
      INT32 _attachNextExt( dmsMBContext *context, dmsExtentInfo *workExt ) ;
      void _detachWorkExt( UINT16 collectionID, BOOLEAN sync = TRUE ) ;

      INT32 _shrinkForward( dmsMBContext* context, dmsExtentInfo* workExtInfo,
                            dmsExtentID extID, dmsOffset offset ) ;
      INT32 _shrinkBackward( dmsMBContext* context, dmsExtentInfo* workExtInfo,
                             dmsExtentID extID, dmsOffset offset ) ;

      INT32 _popRecord( dmsMBContext *context,
                        dmsExtentID extID,
                        dmsOffset offset,
                        INT8 direction = 1 ) ;

      OSS_INLINE void _updateCLStat( dmsMBStatInfo &mbStat, UINT32 totalSize,
                                     const dmsRecordData &recordData ) ;
      OSS_INLINE void _updateWorkExtStat( _dmsExtentInfo &extInfo,
                                          UINT32 totalSize,
                                          const dmsRecordData &recordData ) ;
      OSS_INLINE void _updateStatInfo( dmsMBContext *context,
                                       UINT32 recordSize,
                                       const dmsRecordData &recordData ) ;
      OSS_INLINE BOOLEAN _sizeExceedLimit( dmsMBContext *context,
                                           UINT32 newSize ) ;
      OSS_INLINE BOOLEAN _numExceedLimit( dmsMBContext *context, UINT32 size ) ;
      OSS_INLINE BOOLEAN _overwriteOnExceed( dmsMBContext *context ) ;
      OSS_INLINE void _recLid2ExtLidAndOffset( INT64 logicalID,
                                               dmsExtentID &extID,
                                               dmsOffset &offset ) ;
      OSS_INLINE void _extLidAndOffset2RecLid( dmsExtentID extID,
                                               dmsOffset offset,
                                               INT64 &logicalID ) ;
      OSS_INLINE dmsExtentID _logicID2ExtID( dmsMBContext *context,
                                             INT64 logicalID,
                                             const dmsExtent *&extent ) ;

      INT32 _extractRecLID( dmsMBContext *context,
                            INT64 logicalID,
                            pmdEDUCB *cb,
                            dmsExtentID &extentID,
                            dmsOffset &offset ) ;

      INT32 _recycleExtents( dmsMBContext *context,
                             dmsExtentID targetExtID,
                             INT8 direction ) ;

      INT32 _getPrevRecOffset( UINT16 collectionID,
                               dmsExtentID extentID,
                               dmsOffset offset,
                               dmsOffset &prevOffset );

      INT32 _countRecNumAndSize( UINT16 collectionID,
                                 dmsExtentID extentID,
                                 dmsOffset beginOffset,
                                 dmsOffset endOffset,
                                 UINT32 &recNum,
                                 UINT32 &totalSize ) ;

      INT32 _updateExtentLID( UINT16 mbID,
                              dmsExtentID extID,
                              dmsExtentID extLogicID ) ;

      INT32 _limitProcess( dmsMBContext *context, UINT32 sizeReq,
                           dmsExtentInfo *workExtInfo ) ;

   private:
      dmsCappedCLOptions *_options[ DMS_MME_SLOTS ] ;
      dmsExtentInfo _workExtInfo[ DMS_MME_SLOTS ] ;
      SIZE_REQ_MAP  _sizeReqMap ;
   } ;
   typedef _dmsStorageDataCapped dmsStorageDataCapped ;

   OSS_INLINE dmsExtentInfo* _dmsStorageDataCapped::getWorkExtInfo(UINT16 mbID)
   {
      if ( mbID >= DMS_MME_SLOTS )
      {
         return NULL ;
      }

      return &_workExtInfo[ mbID ] ;
   }

   OSS_INLINE void _dmsStorageDataCapped::_updateCLStat( dmsMBStatInfo &mbStat,
                                                         UINT32 totalSize,
                                                         const dmsRecordData &recordData )
   {
      ++( mbStat._totalRecords ) ;
      mbStat._totalDataFreeSpace -= totalSize ;
      mbStat._totalOrgDataLen += totalSize ;
      mbStat._totalDataLen += totalSize ;
      mbStat._lastCompressRatio =
         (UINT8)( recordData.getCompressRatio() * 100 ) ;
   }

   OSS_INLINE void _dmsStorageDataCapped::_updateWorkExtStat( _dmsExtentInfo &extInfo,
                                                              UINT32 totalSize,
                                                              const dmsRecordData &recordData )
   {
      ++( extInfo._recCount ) ;
      extInfo.recNoInc() ;
      extInfo._freeSpace -= totalSize ;
      if ( DMS_INVALID_OFFSET == extInfo._firstRecordOffset )
      {
         SDB_ASSERT( DMS_INVALID_OFFSET ==
                     extInfo._lastRecordOffset,
                     "last record offset should be invalid" ) ;
         if ( 0 == extInfo._writePos )
         {
            extInfo._firstRecordOffset = DMS_EXTENT_METADATA_SZ ;
            extInfo._lastRecordOffset = DMS_EXTENT_METADATA_SZ ;
         }
         else
         {
            extInfo._firstRecordOffset =
               extInfo._writePos + DMS_EXTENT_METADATA_SZ ;
            extInfo._lastRecordOffset =
               extInfo._writePos + DMS_EXTENT_METADATA_SZ ;
         }
      }
      else
      {
         SDB_ASSERT( DMS_INVALID_OFFSET != extInfo._firstRecordOffset,
                     "first record offset should not be invalid" ) ;
         extInfo._lastRecordOffset =
            extInfo._writePos + DMS_EXTENT_METADATA_SZ ;
      }
      extInfo._writePos =
         ossRoundUpToMultipleX( extInfo._writePos + totalSize, 4 ) ;
   }

   OSS_INLINE void _dmsStorageDataCapped::_updateStatInfo( dmsMBContext *context,
                                                           UINT32 recordSize,
                                                           const dmsRecordData &recordData )
   {
      _updateWorkExtStat( _workExtInfo[ context->mbID() ],
                          recordSize, recordData ) ;
      _updateCLStat( _mbStatInfo[ context->mbID() ], recordSize, recordData ) ;
   }

   OSS_INLINE BOOLEAN _dmsStorageDataCapped::_sizeExceedLimit( dmsMBContext *context,
                                                               UINT32 newSize )
   {
      const dmsMBStatInfo *mbStatInfo = getMBStatInfo( context->mbID() ) ;
      SDB_ASSERT( mbStatInfo, "mbStatInfo should not be NULL" ) ;

      return (((UINT64)mbStatInfo->_totalDataPages << pageSizeSquareRoot()) + newSize)
             > (UINT64)_options[context->mbID()]->_maxSize ;
   }

   OSS_INLINE BOOLEAN _dmsStorageDataCapped::_numExceedLimit( dmsMBContext *context,
                                                              UINT32 newNum )
   {
      const dmsMBStatInfo *mbStatInfo = getMBStatInfo( context->mbID() ) ;
      SDB_ASSERT( mbStatInfo, "mbStatInfo should not be NULL" ) ;

      return ( _options[context->mbID()]->_maxRecNum > 0 &&
               ( ( mbStatInfo->_totalRecords + newNum ) >
                 (UINT64)_options[context->mbID()]->_maxRecNum ) ) ;
   }

   OSS_INLINE BOOLEAN _dmsStorageDataCapped::_overwriteOnExceed( dmsMBContext *context )
   {
      return _options[context->mbID()]->_overwrite ;
   }

   OSS_INLINE void _dmsStorageDataCapped::_recLid2ExtLidAndOffset( INT64 logicalID,
                                                                   dmsExtentID &extID,
                                                                   dmsOffset &offset )
   {
      if ( logicalID < 0 )
      {
         extID = DMS_INVALID_EXTENT ;
         offset = DMS_INVALID_OFFSET ;
      }
      else
      {
         extID = logicalID / DMS_CAP_EXTENT_BODY_SZ ;
         offset = logicalID % DMS_CAP_EXTENT_BODY_SZ + DMS_EXTENT_METADATA_SZ ;
      }
   }

   OSS_INLINE void _dmsStorageDataCapped::_extLidAndOffset2RecLid( dmsExtentID extLID,
                                                                   dmsOffset offset,
                                                                   INT64 &logicalID )
   {
      if ( DMS_INVALID_EXTENT == extLID ||
           offset < (INT32)DMS_EXTENT_METADATA_SZ )
      {
         logicalID = DMS_INVALID_REC_LOGICALID ;
      }
      else
      {
         logicalID = (INT64)extLID * DMS_CAP_EXTENT_BODY_SZ + offset
                     - DMS_EXTENT_METADATA_SZ ;
      }
   }

   OSS_INLINE dmsExtentID _dmsStorageDataCapped::_logicID2ExtID( dmsMBContext *context,
                                                                 INT64 logicalID,
                                                                 const dmsExtent *&extent )
   {
      dmsExtRW extRW ;
      dmsExtentID extentID = DMS_INVALID_EXTENT ;
      dmsExtentID extLID = DMS_INVALID_EXTENT ;
      dmsOffset offset = 0 ;

      dmsExtentInfo *extentInfo =  getWorkExtInfo( context->mbID() ) ;

      if ( logicalID < 0 )
      {
         extentID = DMS_INVALID_EXTENT ;
         goto error ;
      }

      _recLid2ExtLidAndOffset( logicalID, extLID, offset ) ;

      extentID = context->mb()->_firstExtentID ;
      extent = NULL ;
      while ( DMS_INVALID_EXTENT != extentID )
      {
         extRW = extent2RW( extentID, context->mbID() ) ;
         extRW.setNothrow( TRUE ) ;
         extent = extRW.readPtr<dmsExtent>() ;
         if ( !extent || extLID < extent->_logicID )
         {
            extentID = DMS_INVALID_EXTENT ;
            goto error ;
         }
         else if ( extLID == extent->_logicID )
         {
            break ;
         }
         else if ( extentID == extentInfo->getID() )
         {
            extentID = DMS_INVALID_EXTENT ;
            break ;
         }
         else
         {
            extentID = extent->_nextExtent;
         }
      }

   done:
      return extentID ;
   error:
      goto done ;
   }
 }

#endif /* DMSSTORAGE_DATACAPPED_HPP */

