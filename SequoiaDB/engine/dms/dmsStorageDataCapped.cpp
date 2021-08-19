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

   Source File Name = dmsStorageDataCapped.cpp

   Descriptive Name = Data Management Service

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
#include "dmsStorageDataCapped.hpp"
#include "dpsOp2Record.hpp"
#include "pmd.hpp"
#include "dmsTrace.hpp"

#define DMS_CAP_CL_MIN_SZ                 DMS_CAP_EXTENT_SZ
#define DMS_CAP_EXTENT_PAGE_NUM           \
   (DMS_CAP_EXTENT_SZ >> pageSizeSquareRoot())

using namespace bson ;

namespace engine
{
   _dmsStorageDataCapped::_dmsStorageDataCapped( const CHAR* pSuFileName,
                                                 dmsStorageInfo *pInfo,
                                                 _IDmsEventHolder *pEventHolder )
   : _dmsStorageDataCommon( pSuFileName, pInfo, pEventHolder )
   {
      ossMemset( (CHAR *)_options, 0, sizeof(_options) ) ;
      _disableBlockScan() ;
      _isCapped = TRUE;
   }

   _dmsStorageDataCapped::~_dmsStorageDataCapped()
   {
   }

   const CHAR* _dmsStorageDataCapped::_getEyeCatcher() const
   {
      return DMS_DATACAPSU_EYECATCHER ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__ONOPENED, "_dmsStorageDataCapped::_onOpened" )
   INT32 _dmsStorageDataCapped::_onOpened()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__ONOPENED ) ;
      // Traverse all the collections which are being used, get the collection
      // extend option pointer and the the working extents.
      rc = dmsStorageDataCommon::_onOpened() ;
      if ( rc )
      {
         goto error ;
      }

      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS; ++i )
      {
         if ( DMS_IS_MB_INUSE ( _dmsMME->_mbList[i]._flag ) &&
              ( DMS_INVALID_EXTENT != _dmsMME->_mbList[i]._mbOptExtentID ) )
         {
            dmsExtRW extRW = extent2RW( _dmsMME->_mbList[i]._mbOptExtentID,
                                        i ) ;
            extRW.setNothrow( TRUE ) ;
            const dmsOptExtent *extent =
               extRW.readPtr<dmsOptExtent>( 0, pageSize()) ;
            if ( !extent )
            {
               PD_LOG( PDERROR, "Option extent is invalid" ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            _options[i] =
               (dmsCappedCLOptions *)((CHAR *)extent + DMS_OPTEXTENT_HEADER_SZ);

            // Attach the last extent as the working extent.
            if ( DMS_INVALID_EXTENT != _dmsMME->_mbList[i]._lastExtentID )
            {
               rc = _attachWorkExt( i, _dmsMME->_mbList[i]._lastExtentID ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to attach work extent: %d, "
                            "rc: %d", rc ) ;
            }
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__ONOPENED, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__ONCLOSED, "_dmsStorageDataCapped::_onClosed" )
   void _dmsStorageDataCapped::_onClosed()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__ONCLOSED ) ;
      dmsExtentInfo *extInfo = NULL ;
      // Flush all working extents information to mmap.
      for ( UINT16 i = 0; i < DMS_MME_SLOTS; ++i )
      {
         if ( DMS_IS_MB_INUSE( _dmsMME->_mbList[i]._flag ) )
         {
            extInfo = getWorkExtInfo( i ) ;
            if ( DMS_INVALID_EXTENT != extInfo->getID() )
            {
               rc = _syncWorkExtInfo( i ) ;
               if ( rc )
               {
                  PD_LOG( PDWARNING, "Failed to sync working extent "
                          "information for collection[%s], rc: %d",
                          _dmsMME->_mbList[i]._collectionName, rc ) ;
               }
            }
         }
      }

      dmsStorageDataCommon::_onClosed() ;
      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACAPPED__ONCLOSED ) ;
   }

   // After restore from crash, the last extents of each collection may have
   // been updated. So need to re-attach the working extents.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__ONRESTORE, "_dmsStorageDataCapped::_onRestore" )
   void _dmsStorageDataCapped::_onRestore()
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__ONRESTORE ) ;
      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS; ++i )
      {
         if ( DMS_IS_MB_INUSE ( _dmsMME->_mbList[i]._flag ) &&
              ( DMS_INVALID_EXTENT != _dmsMME->_mbList[i]._lastExtentID ) )
         {
            // Attach the last extent as the working extent.
            dmsExtRW extRW = extent2RW( _dmsMME->_mbList[i]._lastExtentID, i ) ;
            extRW.setNothrow( TRUE ) ;
            const dmsExtent *extent = extRW.readPtr<dmsExtent>() ;
            if ( extent && extent->validate( i ) )
            {
               if ( 0 == extent->_recCount )
               {
                  SDB_ASSERT( extent->_lastRecordOffset,
                              "Last record offset is invalid" ) ;
                  _syncWorkExtInfo( i ) ;
               }
               else
               {
                  _attachWorkExt( i, _dmsMME->_mbList[i]._lastExtentID ) ;
               }
            }
         }
      }
      dmsStorageDataCommon::_onRestore() ;
      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACAPPED__ONRESTORE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__ONFLUSHDIRTY, "_dmsStorageDataCapped::_onFlushDirty" )
   INT32 _dmsStorageDataCapped::_onFlushDirty( BOOLEAN force, BOOLEAN sync )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__ONFLUSHDIRTY ) ;
      // For capped collections, flush the working extent information when flush
      // dirty.
      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS; ++i )
      {
         if ( DMS_IS_MB_INUSE( _dmsMME->_mbList[i]._flag ) )
         {
            dmsExtentInfo *workExtInfo = getWorkExtInfo( i ) ;
            dmsExtentID extentID = workExtInfo->_id ;
            // The working extent is invalid before any records insert into a
            // new created collection. So only sync when it's valid.
            if ( DMS_INVALID_EXTENT != extentID )
            {
               dmsExtRW extRW = extent2RW( extentID, i ) ;
               extRW.setNothrow( TRUE ) ;
               const dmsExtent *extent = extRW.readPtr<dmsExtent>() ;
               if ( extent->_recCount != workExtInfo->_recCount ||
                    extent->_lastRecordOffset != workExtInfo->_lastRecordOffset
                    || extent->_freeSpace != (INT32)workExtInfo->_freeSpace )
               {
                  _syncWorkExtInfo( i ) ;
               }
            }
         }
      }
      dmsStorageDataCommon::_onFlushDirty( force, sync ) ;
      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACAPPED__ONFLUSHDIRTY ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__ONCOLLECTIONTRUNCATED, "_dmsStorageDataCapped::_onCollectionTruncated" )
   INT32 _dmsStorageDataCapped::_onCollectionTruncated( dmsMBContext *context )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__ONCOLLECTIONTRUNCATED ) ;
      if ( !context->isMBLock(EXCLUSIVE ) )
      {
         PD_LOG( PDERROR, "Caller must hold exclusive lock on mb" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _workExtInfo[context->mbID()].reset() ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__ONCOLLECTIONTRUNCATED, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__PREPAREADDCOLLECTION, "_dmsStorageDataCapped::_prepareAddCollection" )
   INT32 _dmsStorageDataCapped::_prepareAddCollection( const BSONObj *extOption,
                                                       dmsExtentID &extOptExtent,
                                                       UINT16 &extentPageNum )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__PREPAREADDCOLLECTION ) ;
      UINT16 pageNum = 1 ;
      dmsCappedCLOptions options ;
      dmsExtentID extentID = DMS_INVALID_EXTENT ;

      rc = _parseExtendOptions( extOption, options ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse options, rc: %d", rc ) ;

      // Allocate one page to store possible options of the collection.
      rc = _findFreeSpace( pageNum, extentID, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Allocate metablock expand option extent "
                   "failed, pageNum: %d, rc: %d", pageNum, rc ) ;
      extOptExtent = extentID ;
      extentPageNum = pageNum ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__PREPAREADDCOLLECTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__ONADDCOLLECTION, "_dmsStorageDataCapped::_onAddCollection" )
   INT32 _dmsStorageDataCapped::_onAddCollection( const BSONObj *extOption,
                                                  dmsExtentID extOptExtent,
                                                  UINT32 extentSize,
                                                  UINT16 collectionID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__ONADDCOLLECTION ) ;
      dmsExtRW optExtRW ;
      dmsCappedCLOptions options ;
      dmsOptExtent *optExtent = NULL ;

      rc = _parseExtendOptions( extOption, options ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse options, rc: %d", rc ) ;

      // Init the option page.
      optExtRW = extent2RW( extOptExtent, collectionID ) ;
      optExtRW.setNothrow( TRUE ) ;
      optExtent = optExtRW.writePtr<dmsOptExtent>( 0, extentSize <<
                                                      pageSizeSquareRoot() ) ;
      PD_CHECK( optExtent, SDB_SYS, error, PDERROR,
                "Invalid option extent[%d]", optExtent ) ;
      optExtent->init( extentSize, collectionID ) ;
      optExtent->setOption( (const CHAR *)&options,
                            sizeof( dmsCappedCLOptions )) ;

      // Flush immediately to avoid corruption of this page in crash.
      flushPages( extOptExtent, extentSize, isSyncDeep() ) ;

      rc = optExtent->getOption( (CHAR **)&_options[collectionID], NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Get collection extend option from extent "
                   "failed, rc: %d", rc ) ;
      SDB_ASSERT( _options[collectionID],
                  "Option pointer should not be NULL" ) ;

      // As capped collection dose not support index, always set the index
      // commit flag to true.
      _dmsMME->_mbList[collectionID]._idxCommitFlag = 1 ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__ONADDCOLLECTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__ONALLOCEXTENT, "_dmsStorageDataCapped::_onAllocExtent" )
   void _dmsStorageDataCapped::_onAllocExtent( dmsMBContext *context,
                                               dmsExtent *extAddr,
                                               SINT32 extentID )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__ONALLOCEXTENT ) ;

      SDB_ASSERT( context, "Context should not be NULL" ) ;
      SDB_ASSERT( extAddr, "Extent address should not be NULL" ) ;

      _mbStatInfo[context->mbID()]._totalDataFreeSpace +=
            ( (extAddr->_blockSize) << pageSizeSquareRoot() ) -
            DMS_EXTENT_METADATA_SZ ;

      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACAPPED__ONALLOCEXTENT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__PREPAREINSERTDATA, "_dmsStorageDataCapped::_prepareInsertData" )
   INT32 _dmsStorageDataCapped::_prepareInsertData( const BSONObj &record,
                                                    BOOLEAN mustOID,
                                                    pmdEDUCB *cb,
                                                    dmsRecordData &recordData,
                                                    BOOLEAN &memReallocate,
                                                    INT64 position )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__PREPAREINSERTDATA ) ;
      LogicalIDToInsert logicalID ;
      LogicalIDToInsertEle logicalIDEle( (CHAR *)(&logicalID) ) ;
      CHAR *mergedData = NULL ;
      CHAR *writePos = NULL ;

      if ( !mustOID )
      {
         PD_LOG( PDERROR, "_id is always required by capped "
                 "collection record" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         if ( position >= 0 )
         {
            recordData.setData( record.objdata(), record.objsize() ) ;
            memReallocate = FALSE ;
            goto done ;
         }

         // _id can not be set explicitly. It will be generated inside.
         // So if there is one in the original record, it will be ignored.
         // We don't return error here because the driver always add the _id
         // field for all records...
         INT32 idSize = 0 ;
         UINT32 totalSize = logicalIDEle.size() + record.objsize() ;
         BSONElement ele = record.getField( DMS_ID_KEY_NAME ) ;
         if ( EOO != ele.type() )
         {
            idSize = ele.size() ;
            totalSize -= idSize ;
         }

         // The logical is unknow for now, just reserve the space for it.
         // Its value will be filled in _extentInsertRecord.
         rc = cb->allocBuff( totalSize, &mergedData ) ;
         PD_RC_CHECK( rc, PDERROR, "Allocate memory[size: %u] failed, rc: %d",
                      totalSize, rc ) ;

         // Set the BSONObj length.
         *(UINT32*)mergedData = totalSize ;
         writePos = mergedData + sizeof(UINT32) ;
         ossMemcpy( writePos, logicalIDEle.rawdata(), logicalIDEle.size() ) ;
         writePos += logicalIDEle.size() ;
         // If no _id or _id is the first element of the object.
         if ( 0 == idSize || ( ele.value() == mergedData + sizeof(UINT32) ) )
         {
            ossMemcpy( writePos,
                       record.objdata() + sizeof(UINT32) + idSize,
                       record.objsize() - sizeof(UINT32) - idSize ) ;
         }
         else
         {
            // _id is not at the beginning of the record. It may be at the end
            // of the record, or in the middle.
            const CHAR *copyPos = record.objdata() + sizeof(UINT32) ;
            INT32 copyLen = ele.rawdata() - record.objdata() - sizeof(UINT32) ;
            ossMemcpy( writePos, copyPos, copyLen ) ;
            writePos += copyLen ;
            // Ignore the _id.
            copyPos += copyLen + idSize ;
            INT32 remainLen = record.objsize() -
                              ( copyPos - record.objdata() ) ;
            // There is a terminator at the end of BSONObj.
            SDB_ASSERT( remainLen >= 1,
                        "Remain length should be greater than 0" ) ;
            if ( remainLen > 0 )
            {
               ossMemcpy( writePos, copyPos, remainLen ) ;
            }
         }
         recordData.setData( mergedData, totalSize, UTIL_COMPRESSOR_INVALID, TRUE ) ;
         memReallocate = TRUE ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__PREPAREINSERTDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__FINALRECORDSIZE, "_dmsStorageDataCapped::_finalRecordSize" )
   void _dmsStorageDataCapped::_finalRecordSize( UINT32 &size,
                                                 const dmsRecordData &recordData )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__FINALRECORDSIZE ) ;
      // Append the logic id size. Only do that when the data is not compressed.
      // In case of data compressed, logical id is only stored in the record
      // header, and will be copied out when fetch the data.
      if ( !recordData.isCompressed() )
      {
         size += sizeof( LogicalIDToInsert ) ;
      }

      size += DMS_RECORD_CAP_METADATA_SZ ;
      // record is ALWAYS 4 bytes aligned
      size = OSS_MIN( DMS_RECORD_MAX_SZ, ossAlignX ( size, 4 ) ) ;
      PD_TRACE2 ( SDB__DMSSTORAGEDATACAPPED__FINALRECORDSIZE,
                  PD_PACK_STRING ( "size after align" ),
                  PD_PACK_UINT ( size ) ) ;
      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACAPPED__FINALRECORDSIZE ) ;
   }

   // The record space allocation strategy is as follows:
   // 1. Before reaching the limit( both size and number ), continue to write
   //    forward. If the freespace in the current extent is not enough,
   //    allocate a new one. No recycle needs to be done.
   // 2. If we reach the limit( either size or number ), recycle the eldest
   //    extent.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__ALLOCRECORDSPACE, "_dmsStorageDataCapped::_allocRecordSpace" )
   INT32 _dmsStorageDataCapped::_allocRecordSpace( dmsMBContext *context,
                                                   UINT32 size,
                                                   dmsRecordID &foundRID,
                                                   pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__ALLOCRECORDSPACE ) ;
      dmsExtentID extID = DMS_INVALID_EXTENT ;
      dmsExtentInfo *workExtInfo = NULL ;

      SDB_ASSERT( context, "Context should not be NULL" ) ;
      SDB_ASSERT( cb, "edu cb should not be NULL" ) ;

      workExtInfo = getWorkExtInfo( context->mbID() ) ;
      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold exclusive lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      // If the working extent is invalid, no data extent has been allocated for
      // the collection yet.
      if ( DMS_INVALID_EXTENT == workExtInfo->getID() )
      {
         SDB_ASSERT( DMS_INVALID_EXTENT == context->mb()->_firstExtentID
                     && DMS_INVALID_EXTENT == context->mb()->_lastExtentID,
                     "The first and last extents should be invalid" ) ;
         rc = _allocateExtent( context, DMS_CAP_EXTENT_PAGE_NUM,
                               TRUE, FALSE, &extID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to allocate extent, rc: %d", rc ) ;

         // As the collection lock may have been released in the invoking of
         // _allocateExtent(in _findFreeSpace, to be precise), some one else
         // (in parrallel scenario) may have also reached _allocateExtent, and
         // allocated one extent. That is most likely to happen when doing
         // parrallel insertions. The working extent may have already switched.
         // So we need to check again, and only to switch to the new allocated
         // extent here if the current working extent's free space is not enough.

         // The 2 conditions below:
         // (1) If the working extent is STILL invalid, no body else has done
         //     the attach yet. So we'll do it, attaching to the first extent
         //     of the collection.
         // (3) If the working extent is valid, it means some other thread has
         //     done the attach. So we check if the free space is enough. If not
         //     attach to the next extent(There should be one).
         if ( DMS_INVALID_EXTENT == workExtInfo->getID() ||
              workExtInfo->_freeSpace < size )
         {
            // There should be a next extent. Let switch to it.
            rc = _attachNextExt( context, workExtInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Switch working extent failed, "
                         "rc: %d", rc ) ;
         }
      }
      else
      {
         rc = _limitProcess( context, size, workExtInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Collection limit error, rc: %d", rc ) ;

         if ( workExtInfo->_freeSpace < size )
         {
            rc = _allocateExtent( context, DMS_CAP_EXTENT_PAGE_NUM, TRUE,
                                  FALSE, &extID ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to allocate extent, rc: %d", rc ) ;
            if ( workExtInfo->_freeSpace < size )
            {
               // There should be a next extent. Let switch to it.
               rc = _attachNextExt( context, workExtInfo ) ;
               PD_RC_CHECK( rc, PDERROR, "Switch working extent failed, "
                            "rc: %d", rc ) ;
            }
         }
      }

      foundRID._extent = workExtInfo->getID() ;
      foundRID._offset = workExtInfo->getNextRecOffset() ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__ALLOCRECORDSPACE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // There are some restrictions with insertion by position.
   // If the target extent is not empty, the record can only be inserted just
   // after the last record. That's the same with normal insertion.
   // If the target extent is empty( or has not been allocated yet), it can be
   // inserted into any place in the data area of the extent.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__ALLOCRECORDSPACEBYPOS, "_dmsStorageDataCapped::_allocRecordSpaceByPos" )
   INT32 _dmsStorageDataCapped::_allocRecordSpaceByPos( dmsMBContext *context,
                                                        UINT32 size,
                                                        INT64 position,
                                                        dmsRecordID &foundRID,
                                                        pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__ALLOCRECORDSPACEBYPOS ) ;
      dmsExtentID extLogicalID = DMS_INVALID_EXTENT ;
      dmsExtentID extID = DMS_INVALID_EXTENT ;
      dmsOffset offset = DMS_INVALID_OFFSET ;
      dmsExtentInfo *workExtInfo = NULL ;

      _recLid2ExtLidAndOffset( position, extLogicalID, offset ) ;

      workExtInfo = getWorkExtInfo( context->mbID() ) ;
      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold exclusive lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      // Check if the target extent has been allocated yet.
      if ( DMS_INVALID_EXTENT == workExtInfo->getID() )
      {
         SDB_ASSERT( DMS_INVALID_EXTENT == context->mb()->_firstExtentID
                     && DMS_INVALID_EXTENT == context->mb()->_lastExtentID,
                     "The first and last extents should be invalid" ) ;
         rc = _allocateExtent( context, DMS_CAP_EXTENT_PAGE_NUM,
                               TRUE, FALSE, &extID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to allocate extent, rc: %d", rc ) ;

         if ( DMS_INVALID_EXTENT == workExtInfo->getID() )
         {
            // First extent of the collection. If position is specified( for
            // example, full sync is in progress), need to update the logical
            // extent id accorrding to the position.
            rc = _updateExtentLID( context->mbID(), extID, extLogicalID ) ;
            PD_RC_CHECK( rc, PDERROR, "Update extent logical id to %d "
                         "failed[ %d ]", extLogicalID, rc ) ;
            _mbStatInfo[context->mbID()]._totalDataFreeSpace -=
               offset - DMS_EXTENT_METADATA_SZ ;
         }

         if ( DMS_INVALID_EXTENT == workExtInfo->getID() ||
              workExtInfo->_freeSpace < size )
         {
            // There should be a next extent. Let switch to it.
            rc = _attachNextExt( context, workExtInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Switch working extent failed, "
                         "rc: %d", rc ) ;
            // Adjust the write position to the target place.
            workExtInfo->seek( offset ) ;
         }
      }
      else
      {
         // If the working extent is not invalid, there are two scenarios:
         // 1. The target extent is just the working extent. In this case,
         //    offset should be the same with workExtInfo->getNextRecOffset().
         // 2. The target is not the working extent. Then it can only be the
         //    extent after the working extent, and need to be allocate.
         if ( extLogicalID == workExtInfo->_extLogicID )
         {
            if ( offset != workExtInfo->getNextRecOffset() )
            {
#ifdef _DEBUG
               if ( EDU_TYPE_REPLAGENT == cb->getType() )
               {
                  SDB_ASSERT( FALSE, "Offset is out of range" ) ;
               }
#endif /* _DEBUG */
               PD_LOG( PDERROR, "Record offset[%d] from position %lld dose not "
                                "match next offset[%d] in extent[%d]",
                       offset, position, workExtInfo->getNextRecOffset(),
                       extLogicalID ) ;
               rc = SDB_INVALIDARG ;
               goto done ;
            }
         }
         else if ( extLogicalID == ( workExtInfo->_extLogicID + 1 ) )
         {
            INT64 logicalID = DMS_INVALID_REC_LOGICALID ;
            // Get the first record logical id in the next extent, and check.
            _extLidAndOffset2RecLid( extLogicalID, DMS_EXTENT_METADATA_SZ,
                                     logicalID ) ;
            if ( position != logicalID )
            {
#ifdef _DEBUG
               if ( EDU_TYPE_REPLAGENT == cb->getType() )
               {
                  SDB_ASSERT( FALSE, "Invalid position to insert" ) ;
               }
#endif /* _DEBUG */
               PD_LOG( PDERROR, "Invalid position to insert[ %lld ]",
                       position ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            rc = _limitProcess( context, size, workExtInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Collection limit error, rc: %d", rc ) ;

            if ( workExtInfo->_freeSpace < size )
            {
               rc = _allocateExtent( context, DMS_CAP_EXTENT_PAGE_NUM, TRUE,
                                     FALSE, &extID ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to allocate extent, rc: %d", rc ) ;
               if ( workExtInfo->_freeSpace < size )
               {
                  // There should be a next extent. Let switch to it.
                  rc = _attachNextExt( context, workExtInfo ) ;
                  PD_RC_CHECK( rc, PDERROR, "Switch working extent failed, "
                               "rc: %d", rc ) ;
               }
            }
         }
         else
         {
            PD_LOG( PDERROR, "Extent logical id[ %d ] from position[ %lld ] "
                    "is invalid", extLogicalID, position ) ;
            rc = SDB_INVALIDARG ;
            goto done ;
         }
      }

      foundRID._extent = workExtInfo->getID() ;
      foundRID._offset = workExtInfo->getNextRecOffset() ;
      SDB_ASSERT( workExtInfo->_extLogicID == extLogicalID, "Extent logical "
                  "id is not as expected" ) ;
      SDB_ASSERT( foundRID._offset == offset, "Offset for record is not as "
                  "expected" ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__ALLOCRECORDSPACEBYPOS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED_EXTRACTDATA, "_dmsStorageDataCapped::extractData" )
   INT32 _dmsStorageDataCapped::extractData( const dmsMBContext *mbContext,
                                             const dmsRecordRW &recordRW,
                                             pmdEDUCB *cb,
                                             dmsRecordData &recordData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED_EXTRACTDATA ) ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;
      const dmsCappedRecord *pRecord= recordRW.readPtr<dmsCappedRecord>() ;

      recordData.reset() ;

      if ( !mbContext->isMBLock() )
      {
         PD_LOG( PDERROR, "MB Context must be locked" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      recordData.setData( pRecord->getData(), pRecord->getDataLength(),
                          UTIL_COMPRESSOR_INVALID, TRUE ) ;

      if ( pRecord->isCompressed() )
      {
// Currently compression is not supported for capped collection.
#if 0
         /*
          * It's a little complicated in compression case. The original record
          * without logical ID is compressed, and the logical id is stored in
          * the header of the record. We need to combine them as a single
          * complete BSON object here. So we need to allocate the space before
          * uncompression the data, and reserve the space for logical id. After
          * the uncompression, reformat them.
          *
          *  <--reserve-->|<--uncompressed object--->
          *  ----------------------------------------
          * || lid space || len |       items       ||
          *  ----------------------------------------
          * After format, it should be like:
          *  ----------------------------------------
          * || len || lid object + original objects ||
          * -----------------------------------------
          * Logical id is from the record header.
          * Then it can be translated as a BSON object directly.
          */
         CHAR *buffer = NULL ;
         logicIdToInsert logicID ;
         logicIdToInsertEle logicIDEle( (CHAR *)&logicID ) ;
         const CHAR *uncompressData = NULL ;
         INT32 unCompressDataLen = 0 ;
         utilCompressor *compressor =
            _compressorEntry[ mbContext->mbID() ].getCompressor() ;
         UINT32 totalLen = 0 ;
         rc = compressor->getUncompressedLen( pRecord->getData(),
                                              pRecord->getDataLength(),
                                              totalLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get record uncompress length, "
                      "rc: %d", rc ) ;

         totalLen += sizeof( logicIdToInsert ) ;
         // The buffer will be released when reallocation or when the EDU is
         // destroyed.
         buffer = cb->getUncompressBuff( totalLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get uncompress buffer, "
                      "requested size: %u, rc: %d", totalLen, rc ) ;

         uncompressData = buffer + sizeof( logicIdToInsert ) ;
         unCompressDataLen = totalLen - sizeof( logicIdToInsert ) ;

         rc = dmsUncompress( cb, &_compressorEntry[ mbContext->mbID() ],
                             pRecord->getData(), pRecord->getDataLength(),
                             &uncompressData, &unCompressDataLen ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to uncompress data, rc: %d", rc ) ;
            goto error ;
         }
         /// check the length
         if ( unCompressDataLen != *(INT32*)uncompressData )
         {
            PD_LOG( PDERROR, "Uncompress data length[%d] does not match "
                    "real length[%d]", unCompressDataLen,
                    *(INT32*)uncompressData ) ;
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }

         // Append the logical id.
         logicID.setID( pRecord->getLogicID() ) ;
         totalLen = sizeof( logicIdToInsert ) + unCompressDataLen ;
         *(UINT32*)buffer = totalLen ;
         ossMemcpy( buffer + sizeof( UINT32 ), logicIDEle.rawdata(),
                    logicIDEle.size() ) ;

         recordData.setData( buffer, totalLen, FALSE, FALSE ) ;
#endif
      }
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_READ, 1 ) ;
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_READ, 1 ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED_EXTRACTDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__OPERATIONPERMCHK, "_dmsStorageDataCapped::_operationPermChk" )
   INT32 _dmsStorageDataCapped::_operationPermChk( DMS_ACCESS_TYPE accessType )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__OPERATIONPERMCHK ) ;

      switch ( accessType )
      {
         case DMS_ACCESS_TYPE_QUERY:
         case DMS_ACCESS_TYPE_FETCH:
         case DMS_ACCESS_TYPE_INSERT:
         case DMS_ACCESS_TYPE_DELETE:
         case DMS_ACCESS_TYPE_TRUNCATE:
         case DMS_ACCESS_TYPE_POP:
            break ;
         default:
            PD_LOG( PDERROR, "Operation type[ %d ] is incompatible with "
                    "capped collection", accessType ) ;
            rc = SDB_OPERATION_INCOMPATIBLE ;
            goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__OPERATIONPERMCHK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // Check if the current working extent has enough free space for the
   // record.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__ONALLOCSPACEREADY, "_dmsStorageDataCapped::_onAllocSpaceReady" )
   void _dmsStorageDataCapped::_onAllocSpaceReady( dmsContext *context,
                                                    BOOLEAN &doit )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__ONALLOCSPACEREADY ) ;

      // If the context is NULL, the operation should be creating collection.
      if ( !context )
      {
         doit = TRUE ;
      }
      else
      {
         dmsExtentInfo *extInfo = getWorkExtInfo( context->mbID() ) ;
         dmsExtentID workExtID = extInfo->getID() ;

         if ( DMS_INVALID_EXTENT == workExtID )
         {
            doit = TRUE ;
         }
         else
         {
            // Check if this is the last extent.
            dmsExtRW extRW = extent2RW( workExtID, context->mbID() );
            extRW.setNothrow( TRUE ) ;
            const dmsExtent *extent = extRW.readPtr<dmsExtent>() ;
            SDB_ASSERT( extent, "Extent is invalid" ) ;
            // Do the allocation if we are using the last extent.
            doit = ( DMS_INVALID_EXTENT == extent->_nextExtent ) ? TRUE : FALSE ;
         }
      }
      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACAPPED__ONALLOCSPACEREADY ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__RECYCLEWORKEXT, "_dmsStorageDataCapped::_recycleWorkExt" )
   INT32 _dmsStorageDataCapped::_recycleWorkExt( dmsMBContext *context,
                                                 dmsExtentID extID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__RECYCLEWORKEXT ) ;
      UINT16 mbID = context->mbID() ;
      dmsExtRW extRW ;
      dmsExtent *extent = NULL ;
      UINT32 recNum = 0 ;
      UINT32 totalSize = 0 ;
      INT32 currExtLID = 0 ;
      dmsExtentInfo *extInfo = getWorkExtInfo( mbID ) ;

      extRW = extent2RW( extID, mbID ) ;
      extRW.setNothrow( TRUE ) ;
      extent = extRW.writePtr<dmsExtent>() ;
      if ( !extent )
      {
         PD_LOG( PDERROR, "Invalid extent: %d", extID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _countRecNumAndSize( context->mbID(), extID,
                                extInfo->_firstRecordOffset,
                                extInfo->_lastRecordOffset, recNum,
                                totalSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Count record number and size failed[ %d ]",
                   rc ) ;

      // When recycling the working extent, it means there is only one extent in
      // the collection.
      // Initialize the extent, update the statistic information in _mbStatInfo
      // and work extent info. Increase the extent logical id.
      currExtLID = extent->_logicID ;
      extent->init( DMS_CAP_EXTENT_PAGE_NUM, mbID, DMS_CAP_EXTENT_SZ ) ;
      extent->_logicID = currExtLID + 1 ;

      _mbStatInfo[mbID]._totalRecords -= extInfo->_recCount ;
      _mbStatInfo[mbID]._rcTotalRecords.sub( extInfo->_recCount ) ;
      _mbStatInfo[mbID]._totalDataLen -= totalSize ;
      _mbStatInfo[mbID]._totalOrgDataLen -= totalSize ;
      _mbStatInfo[mbID]._totalDataFreeSpace = DMS_CAP_EXTENT_BODY_SZ ;
      SDB_ASSERT( 0 == _mbStatInfo[mbID]._totalRecords,
                  "Total records should be 0" ) ;
      SDB_ASSERT( 0 == _mbStatInfo[mbID]._totalDataLen,
                  "Total data length should be 0" ) ;
      SDB_ASSERT( 0 == _mbStatInfo[mbID]._totalOrgDataLen,
                  "Total original data length should be 0" ) ;

      rc = _attachWorkExt( mbID, extID ) ;
      PD_RC_CHECK( rc, PDERROR, "Attach work extent[%d] failed[%d]",
                   extID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__RECYCLEWORKEXT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__RECYCLEACTIVEEXT, "_dmsStorageDataCapped::_recycleActiveExt" )
   INT32 _dmsStorageDataCapped::_recycleActiveExt( dmsMBContext *context,
                                                   dmsExtentID extID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__RECYCLEACTIVEEXT ) ;
      UINT16 mbID = context->mbID() ;
      dmsExtRW extRW ;
      dmsExtent *extent = NULL ;
      UINT32 recNum = 0 ;
      UINT32 totalSize = 0 ;

      extRW = extent2RW( extID, mbID ) ;
      extRW.setNothrow( TRUE ) ;
      extent = extRW.writePtr<dmsExtent>() ;
      if ( !extent )
      {
         PD_LOG( PDERROR, "Invalid extent: %d", extID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _countRecNumAndSize( mbID, extID, extent->_firstRecordOffset,
                                extent->_lastRecordOffset,
                                recNum, totalSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Count record number and size failed[ %d ]",
                   rc ) ;

      _mbStatInfo[mbID]._totalRecords -= recNum ;
      _mbStatInfo[mbID]._rcTotalRecords.sub( recNum ) ;
      _mbStatInfo[mbID]._totalDataFreeSpace -= extent->_freeSpace ;
      _mbStatInfo[mbID]._totalOrgDataLen -= totalSize ;
      _mbStatInfo[mbID]._totalDataLen -= totalSize ;

      rc = _freeExtent( context, extID ) ;
      PD_RC_CHECK( rc, PDERROR, "Free extent[%d] failed: %d", extID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__RECYCLEACTIVEEXT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__PARSEEXTENTOPTIONS, "_dmsStorageDataCapped::_parseExtendOptions" )
   INT32 _dmsStorageDataCapped::_parseExtendOptions( const BSONObj *extOptions,
                                                     dmsCappedCLOptions &options )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__PARSEEXTENTOPTIONS ) ;

      try
      {
         if ( !extOptions || !extOptions->valid() )
         {
            PD_LOG( PDERROR, "Only capped collection with valid options[Capped/"
                    "Size/Max] can be created on capped collection space" ) ;
            rc = SDB_OPERATION_INCOMPATIBLE ;
            goto error ;
         }

         // Traverse the option object to check.
         {
            BOOLEAN maxSizeFound = FALSE ;
            BOOLEAN maxRecNumFound = FALSE ;
            BOOLEAN overwriteFound = FALSE ;
            INT64 maxSize = 0 ;
            INT64 maxRecNum = 0 ;
            BSONObjIterator itr( *extOptions ) ;
            while ( itr.more() )
            {
               BSONElement ele = itr.next() ;
               const CHAR *fieldName = ele.fieldName() ;
               if ( 0 == ossStrcmp( fieldName, FIELD_NAME_SIZE ) )
               {
                  if ( !ele.isNumber())
                  {
                     PD_LOG( PDERROR, "Invalid type[%d] for option Size",
                             ele.type() ) ;
                     rc = SDB_INVALIDARG ;
                     goto error ;
                  }
                  maxSize = ele.numberLong() ;

                  if ( maxSize < DMS_CAP_CL_MIN_SZ ||
                       (UINT64)maxSize > DMS_MAX_SZ( pageSize() ) )
                  {
                     PD_LOG( PDERROR, "Invalid size[%lld] of capped collection",
                             maxSize ) ;
                     rc = SDB_INVALIDARG ;
                     goto error ;
                  }
                  maxSizeFound = TRUE ;
                  options._maxSize = maxSize ;
               }
               else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_MAX ) )
               {
                  if ( !ele.isNumber() )
                  {
                     PD_LOG( PDERROR, "Invalid type[%d] for option Max",
                             ele.type() ) ;
                     rc = SDB_INVALIDARG ;
                     goto error ;
                  }
                  maxRecNum = ele.numberLong() ;

                  if ( maxRecNum < 0 )
                  {
                     PD_LOG( PDERROR, "Option value[%lld] for Max should not "
                             "be negative", maxRecNum ) ;
                     rc = SDB_INVALIDARG ;
                     goto error ;
                  }
                  maxRecNumFound = TRUE ;
                  options._maxRecNum = maxRecNum ;
               }
               else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_OVERWRITE ) )
               {
                  if ( !ele.isBoolean() )
                  {
                     PD_LOG( PDERROR, "Invalid type[%d] for option OverWrite",
                             ele.type() ) ;
                     rc = SDB_INVALIDARG ;
                     goto error ;
                  }
                  overwriteFound = TRUE ;
                  options._overwrite = ele.boolean() ;
               }
               else
               {
                  PD_LOG( PDERROR, "Unsupported capped collection option: %s",
                          fieldName ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
            }

            if ( !maxSizeFound || !maxRecNumFound || !overwriteFound )
            {
               PD_LOG( PDERROR, "Max/Size/OverWrite not specified "
                       "for capped collection" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Unexpected exception happened: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__PARSEEXTENTOPTIONS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__RECYCLEEXTENT, "_dmsStorageDataCapped::_recycleOneExtent" )
   INT32 _dmsStorageDataCapped::_recycleOneExtent( dmsMBContext *context )
   {
      // Extent will be recycled when the size or record number threshold is hit
      // or when all the records in the extent are popped.
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__RECYCLEEXTENT ) ;
      dmsExtentID extentID = DMS_INVALID_EXTENT ;
      dmsExtentInfo *workExtInfo = getWorkExtInfo( context->mbID() ) ;

      // Always recycle the eldest extent. There are two scenarios:
      // 1. Only one extent in the collection.
      // 2. More than one extents in the collection.
      // The first scenario is a little complicated, as it's the working extent,
      // so some sync or cleaning has to be done.
      extentID = context->mb()->_firstExtentID ;
      if ( extentID == workExtInfo->getID() )
      {
         rc = _recycleWorkExt( context, extentID ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to recycle working extent, rc: %d", rc ) ;
      }
      else
      {
         rc = _recycleActiveExt( context, extentID ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to recycle active extent, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__RECYCLEEXTENT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__EXTENTINSERTRECORD, "_dmsStorageDataCapped::_extentInsertRecord" )
   INT32 _dmsStorageDataCapped::_extentInsertRecord( dmsMBContext *context,
                                                     dmsExtRW &extRW,
                                                     dmsRecordRW &recordRW,
                                                     const dmsRecordData &recordData,
                                                     UINT32 recordSize,
                                                     pmdEDUCB *cb,
                                                     BOOLEAN isInsert )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__EXTENTINSERTRECORD ) ;
      dmsCappedRecord *pRecord = NULL ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;
      dmsExtentInfo *workExtInfo = getWorkExtInfo( context->mbID() ) ;

      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      if ( !recordData.isCompressed() &&
           recordData.len() < DMS_MIN_RECORD_DATA_SZ )
      {
         PD_LOG( PDERROR, "Bson obj size[%d] is invalid", recordData.len() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      pRecord = recordRW.writePtr<dmsCappedRecord>( recordSize ) ;
      // Set the record information. Logical id will be added in setData.
      pRecord->setNormal() ;
      pRecord->resetAttr() ;
      pRecord->setSize( recordSize ) ;
      pRecord->setRecordNo( workExtInfo->currentRecNo() + 1 ) ;
      {
         // Force set the logical id in the record. Logical id is always at the
         // beginning of the record.
         LogicalIDToInsertEle ele( (CHAR *)( recordData.data() +
                                             sizeof(UINT32) ) ) ;
         INT64 *lidPtr = (INT64 *)ele.value() ;
         *lidPtr = workExtInfo->getRecordLogicID() ;
         pRecord->setLogicalID( *lidPtr ) ;
      }

      pRecord->setData( recordData ) ;

      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;

      _updateStatInfo( context, recordSize, recordData ) ;

      // If this is the first record in the extent, update the
      // _firstRecordOffset in the extent header.
      if ( 1 == workExtInfo->_recCount )
      {
         dmsExtent *extent = extRW.writePtr<dmsExtent>() ;
         extent->_firstRecordOffset = workExtInfo->_firstRecordOffset ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__EXTENTINSERTRECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageDataCapped::_extentUpdatedRecord( dmsMBContext *context,
                                                      dmsExtRW &extRW,
                                                      dmsRecordRW &recordRW,
                                                      const dmsRecordData &recordData,
                                                      const BSONObj &newObj,
                                                      pmdEDUCB *cb,
                                                      IDmsOprHandler *pHandler,
                                                      utilUpdateResult *pResult )
   {
      SDB_ASSERT( FALSE, "Should not be here" ) ;
      return SDB_OPERATION_INCOMPATIBLE ;
   }

   // Only allowed to remove the last record.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__EXTENTREMOVERECORD, "_dmsStorageDataCapped::_extentRemoveRecord" )
   INT32 _dmsStorageDataCapped::_extentRemoveRecord( dmsMBContext *context,
                                                     dmsExtRW &extRW,
                                                     dmsRecordRW &recordRW,
                                                     pmdEDUCB *cb,
                                                     BOOLEAN decCount )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__EXTENTREMOVERECORD ) ;
      dmsExtentInfo *workExtInfo = getWorkExtInfo( context->mbID() ) ;
      dmsRecordID lastRID( workExtInfo->_id,
                           workExtInfo->_lastRecordOffset ) ;

      // If the working extent is empty, the record to be deleted should be in
      // the previous extent(if any). So recycle the current working extent, and
      // swith to the previous one.
      if ( 0 == workExtInfo->_recCount &&
           ( context->mb()->_firstExtentID != context->mb()->_lastExtentID ) )
      {
         dmsExtRW extRW = extent2RW( workExtInfo->_id, context->mbID() ) ;
         extRW.setNothrow( TRUE ) ;
         const dmsExtent* extent = extRW.readPtr<dmsExtent>() ;
         SDB_ASSERT( DMS_INVALID_EXTENT != extent->_prevExtent,
                     "Previous extent is invalid" ) ;
         rc = _syncWorkExtInfo( context->mbID() ) ;
         PD_RC_CHECK( rc, PDERROR, "Sync working extent info failed, rc: %d",
                      rc ) ;
         rc = _recycleExtents( context, extent->_prevExtent, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to recycle extents, rc: %d", rc ) ;
         rc = _attachWorkExt( context->mbID(), extent->_prevExtent ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to attach to new working extent, rc: %d", rc ) ;
         lastRID._extent = workExtInfo->_id ;
         lastRID._offset = workExtInfo->_lastRecordOffset ;
      }

      if ( recordRW.getRecordID() != lastRID )
      {
#ifdef _DEBUG
         if ( EDU_TYPE_REPLAGENT == cb->getType() )
         {
            SDB_ASSERT( FALSE, "Record to be removed is not the last one in "
                               "the capped collection" ) ;
         }
#endif /* _DEBUG */
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Only allowed to delete the last record in "
                 "capped collection, rc: %d", rc ) ;
         goto error ;
      }

      // Actually, we tigger a pop operation on the last record.
      rc = _popRecord( context, lastRID._extent, lastRID._offset, -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Pop record[extent: %d, offset: %d] failed, "
                   "rc: %d",  lastRID._extent, lastRID._offset, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__EXTENTREMOVERECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__ONINSERTFAIL, "_dmsStorageDataCapped::_onInsertFail" )
   INT32 _dmsStorageDataCapped::_onInsertFail( dmsMBContext *context,
                                               BOOLEAN hasInsert,
                                               dmsRecordID rid,
                                               SDB_DPSCB *dpscb,
                                               ossValuePtr dataPtr,
                                               pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__ONINSERTFAIL ) ;

      if ( hasInsert )
      {
         // Revert all what has been done in _extentInsertRecord. This is done
         // by popping the last record backward.
         dmsExtRW extRW ;
         dmsExtent *extent = NULL ;
         INT64 logicalID = -1 ;

         extRW = extent2RW( rid._extent, context->mbID() ) ;
         extRW.setNothrow( TRUE ) ;
         extent = extRW.writePtr<dmsExtent>() ;
         if ( !extent )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid extent: %d, rc: %d", rid._extent, rc ) ;
            goto error ;
         }

         logicalID = (INT64)extent->_logicID * DMS_CAP_EXTENT_BODY_SZ +
                     rid._offset - DMS_EXTENT_METADATA_SZ ;

         rc = popRecord( context, logicalID, cb, dpscb, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Remove the inserted record failed[ %d ]",
                      rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__ONINSERTFAIL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED_POSTDATARESTORED, "_dmsStorageDataCapped::postDataRestored" )
   INT32 _dmsStorageDataCapped::postDataRestored( dmsMBContext * context )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED_POSTDATARESTORED ) ;
      if ( DMS_INVALID_EXTENT != context->mb()->_lastExtentID )
      {
         _attachWorkExt( context->mbID(), context->mb()->_lastExtentID ) ;
      }

      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACAPPED_POSTDATARESTORED ) ;
      return SDB_OK ;
   }

   // Flush the extent information to mmap.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__SYNCWORKEXTINFO, "_dmsStorageDataCapped::_syncWorkExtInfo" )
   INT32 _dmsStorageDataCapped::_syncWorkExtInfo( UINT16 collectionID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__SYNCWORKEXTINFO ) ;
      dmsExtentID extID = DMS_INVALID_EXTENT ;
      dmsExtRW extRW ;
      dmsExtent *extent = NULL ;
      dmsExtentInfo *extInfo = getWorkExtInfo( collectionID ) ;

      extID = extInfo->getID() ;
      if ( DMS_INVALID_EXTENT == extID )
      {
         goto done ;
      }

      extRW = extent2RW( extID, collectionID ) ;
      extRW.setNothrow( TRUE ) ;
      extent = extRW.writePtr<dmsExtent>() ;
      if ( !extent )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid extent: %d, rc: %d", extID, rc ) ;
         goto error ;
      }

      extent->_recCount = extInfo->_recCount ;
      extent->_freeSpace = extInfo->_freeSpace ;
      extent->_firstRecordOffset = extInfo->_firstRecordOffset ;
      extent->_lastRecordOffset = extInfo->_lastRecordOffset ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__SYNCWORKEXTINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__SWITCHWORKEXT, "_dmsStorageDataCapped::_switchWorkExt" )
   INT32 _dmsStorageDataCapped::_switchWorkExt( dmsMBContext *context,
                                                dmsExtentID extID )
   {
      // Detach the current extent, if there is one, flush the meta data.
      // Then attach to the specified new extent.
      // In case of any error, need to restore the original state.
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__SWITCHWORKEXT ) ;
      BOOLEAN detached = FALSE ;
      dmsExtentID currWorkExt = DMS_INVALID_EXTENT ;
      dmsExtentInfo *workExtInfo = getWorkExtInfo( context->mbID() ) ;
      SDB_ASSERT( workExtInfo, "Work extent info pointer should not be NULL" ) ;

      currWorkExt = workExtInfo->getID() ;

      if ( DMS_INVALID_EXTENT == extID )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Extent to swith to is invalid, rc: %d", rc ) ;
         goto error ;
      }

      if ( !(context->isMBLock( EXCLUSIVE ) ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold exclusive lock on mb: %d, rc: %d",
                 context->mbID(), rc ) ;
         goto error ;
      }

      // If currently attached to a valid extent, need to detach first.
      if ( DMS_INVALID_EXTENT != currWorkExt )
      {
         _detachWorkExt( context->mbID(), TRUE ) ;
         detached = TRUE ;
      }

      rc = _attachWorkExt( context->mbID(), extID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to attach to new working extent, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__SWITCHWORKEXT, rc ) ;
      return rc ;
   error:
      // In case of error, may need to switch back to the original extent.
      if ( detached )
      {
         INT32 rcTmp = _attachWorkExt( context->mbID(), currWorkExt ) ;
         SDB_ASSERT( SDB_OK == rcTmp, "Switch to original extent failed" ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Failed to switch to original extent failed, "
                    "rc: %d", rcTmp ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__ATTACHWORKEXT, "_dmsStorageDataCapped::_attachWorkExt" )
   INT32 _dmsStorageDataCapped::_attachWorkExt( UINT16 collectionID,
                                                dmsExtentID extID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__ATTACHWORKEXT ) ;
      dmsExtRW extRW ;
      const dmsExtent *extent = NULL ;
      dmsExtentInfo *workExtInfo = getWorkExtInfo( collectionID ) ;

      SDB_ASSERT( workExtInfo, "Work extent info pointer should not be NULL" ) ;

      if ( DMS_INVALID_EXTENT == extID )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid extent to attach, rc: %d", rc ) ;
         goto error ;
      }

      extRW = extent2RW( extID, collectionID ) ;
      extRW.setNothrow( TRUE ) ;
      extent = extRW.readPtr<dmsExtent>() ;
      if ( !extent )
      {
         PD_LOG( PDERROR, "Invalid extent: %d", extID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      workExtInfo->_id = extID ;
      workExtInfo->_extLogicID = extent->_logicID ;
      workExtInfo->_recCount = extent->_recCount ;
      workExtInfo->_freeSpace = extent->_freeSpace ;
      workExtInfo->_firstRecordOffset = extent->_firstRecordOffset ;
      workExtInfo->_lastRecordOffset = extent->_lastRecordOffset ;
      if ( (INT32)DMS_INVALID_OFFSET == extent->_lastRecordOffset )
      {
         workExtInfo->_writePos = 0 ;
         workExtInfo->_recNo = 0 ;
      }
      else
      {
         SDB_ASSERT( extent->_lastRecordOffset >=
                     (INT32)DMS_METAEXTENT_HEADER_SZ,
                     "Last record offset is invalid" ) ;
         dmsRecordID recordID( extID, extent->_lastRecordOffset ) ;
         dmsRecordRW recordRW = record2RW( recordID, collectionID ) ;
         recordRW.setNothrow( TRUE ) ;
         const dmsCappedRecord *record = recordRW.readPtr<dmsCappedRecord>() ;
         if ( !record || !record->isNormal() )
         {
            PD_LOG( PDERROR, "Invalid record" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         workExtInfo->_writePos =
            ossRoundUpToMultipleX( extent->_lastRecordOffset + record->getSize()
                                   - DMS_EXTENT_METADATA_SZ, 4 ) ;
         workExtInfo->_recNo = record->getRecordNo() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__ATTACHWORKEXT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__ATTACHNEXTEXT, "_dmsStorageDataCapped::_attachNextExt" )
   INT32 _dmsStorageDataCapped::_attachNextExt( dmsMBContext *context,
                                                dmsExtentInfo *workExt )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__ATTACHNEXTEXT ) ;
      dmsExtentID currExt = workExt->getID() ;
      dmsExtentID nextExt =  DMS_INVALID_EXTENT ;
      dmsExtRW extRW ;

      // If the working extent is invalid, there is only one extent for the
      // collection now.
      if ( DMS_INVALID_EXTENT == currExt )
      {
         SDB_ASSERT( context->mb()->_firstExtentID ==
                     context->mb()->_lastExtentID,
                     "mb first and last extent should be the same" ) ;

         nextExt = context->mb()->_firstExtentID ;
      }
      else
      {
         extRW = extent2RW( currExt, context->mbID() ) ;
         extRW.setNothrow( TRUE ) ;
         const dmsExtent *extent = extRW.readPtr<dmsExtent>() ;
         if ( !extent || !extent->validate( context->mbID() ) )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid extent[%d]", currExt ) ;
            goto error ;
         }

         nextExt = extent->_nextExtent ;
         SDB_ASSERT( DMS_INVALID_EXTENT != nextExt,
                     "Next extent should not be invalid" ) ;
      }

      rc = _switchWorkExt( context, nextExt ) ;
      PD_RC_CHECK( rc, PDERROR, "Switch working extent failed, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__ATTACHNEXTEXT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__DETACHWORKEXT, "_dmsStorageDataCapped::_detachWorkExt" )
   void _dmsStorageDataCapped::_detachWorkExt( UINT16 collectionID,
                                               BOOLEAN sync )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__DETACHWORKEXT ) ;
      dmsExtentID workExtID = DMS_INVALID_EXTENT ;
      dmsExtentInfo *workExtInfo = getWorkExtInfo( collectionID ) ;

      SDB_ASSERT( workExtInfo, "Work extent info pointer should not be NULL" ) ;
      workExtID = workExtInfo->getID() ;

      if ( DMS_INVALID_EXTENT == workExtID )
      {
         goto done ;
      }

      if ( sync )
      {
         _syncWorkExtInfo( collectionID ) ;
      }

      workExtInfo->reset() ;

   done:
      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACAPPED__DETACHWORKEXT ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__POPFROMACTIVEEXTENT, "_dmsStorageDataCapped::_popFromActiveExt" )
   INT32 _dmsStorageDataCapped::_popFromActiveExt( dmsMBContext *context,
                                                   dmsExtentID extentID,
                                                   dmsOffset offset,
                                                   INT8 direction )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__POPFROMACTIVEEXTENT ) ;
      dmsExtRW extRW ;
      dmsExtent *extent = NULL ;
      UINT32 recNum = 0 ;
      UINT32 totalSize = 0 ;
      dmsOffset startOffset = DMS_INVALID_OFFSET ;
      dmsOffset endOffset = DMS_INVALID_OFFSET ;
      BOOLEAN popAll = FALSE ;

      extRW = extent2RW( extentID, context->mbID() ) ;
      extRW.setNothrow( TRUE ) ;
      extent = extRW.writePtr<dmsExtent>() ;
      if ( !extent || !extent->validate( context->mbID() ) )
      {
         PD_LOG( PDERROR, "Invalid extent[%d]", extentID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( direction >= 0 )
      {
         startOffset = extent->_firstRecordOffset ;
         endOffset = offset ;
         popAll = ( offset == extent->_lastRecordOffset ) ;
      }
      else
      {
         startOffset = offset ;
         endOffset = extent->_lastRecordOffset ;
         popAll = ( offset == extent->_firstRecordOffset ) ;
      }

      rc = _countRecNumAndSize( context->mbID(), extentID, startOffset,
                                endOffset, recNum, totalSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Count record number and size "
                   "failed[ %d ]", rc ) ;

      if ( popAll )
      {
         extent->_firstRecordOffset = DMS_INVALID_OFFSET ;
         extent->_lastRecordOffset = DMS_INVALID_OFFSET ;
         if ( direction < 0 )
         {
            extent->_freeSpace += totalSize ;
         }
      }
      else if ( direction >= 0 )
      {
         extent->_firstRecordOffset += totalSize ;
      }
      else
      {
         dmsOffset prevOffset = DMS_INVALID_OFFSET ;
         rc = _getPrevRecOffset( context->mbID(), extentID,
                                 offset, prevOffset ) ;
         PD_RC_CHECK( rc, PDERROR, "Get previous record offset failed[ %d ]",
                      rc ) ;
         extent->_lastRecordOffset = prevOffset ;
         extent->_freeSpace += totalSize ;
         _mbStatInfo[ context->mbID() ]._totalDataFreeSpace += totalSize ;
      }

      extent->_recCount -= recNum ;
      _mbStatInfo[ context->mbID() ]._totalRecords -= recNum ;
      _mbStatInfo[ context->mbID() ]._rcTotalRecords.sub( recNum ) ;
      _mbStatInfo[ context->mbID() ]._totalOrgDataLen -= totalSize ;
      _mbStatInfo[ context->mbID() ]._totalDataLen -= totalSize ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__POPFROMACTIVEEXTENT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__SHRINKFORWARD, "_dmsStorageDataCapped::_shrinkForward" )
   INT32 _dmsStorageDataCapped::_shrinkForward( dmsMBContext* context,
                                                dmsExtentInfo* workExtInfo,
                                                dmsExtentID extID,
                                                dmsOffset offset )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__SHRINKFORWARD ) ;
      BOOLEAN popWorkExt = FALSE ;
      dmsOffset expectOffset = DMS_INVALID_OFFSET ;

      // If pop from the working extent, first we sync the extent meta data from
      // cache to extent. Then do the pop operation in the target extent, and
      // attach work extent again.
      popWorkExt = ( workExtInfo->getID() == extID ) ? TRUE : FALSE ;
      if ( popWorkExt )
      {
         rc = _syncWorkExtInfo( context->mbID() ) ;
         PD_RC_CHECK( rc, PDERROR, "Sync working extent info failed, rc: %d",
                      rc ) ;
         // Save the next record offset for resume.
         expectOffset = workExtInfo->getNextRecOffset() ;
      }

      // Recycle extents which are before the target extent.
      if ( extID != context->mb()->_firstExtentID )
      {
         rc = _recycleExtents( context, extID, 1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to recycle extents, rc: %d", rc ) ;
      }

      {
         dmsExtRW extRW ;
         dmsExtent *extent = NULL ;
         extRW = extent2RW( extID, context->mbID() ) ;
         extRW.setNothrow( TRUE ) ;
         extent = extRW.writePtr<dmsExtent>() ;
         if ( !extent || !extent->validate( context->mbID() ))
         {
            PD_LOG( PDERROR, "Invalid extent[%d]", extID ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         // If we are going to pop all the records in the target extent, and
         // it's not the last remaining extent of the collection, recycle the
         // extent. The first extent will be the one after this extent.
         if ( (offset == extent->_lastRecordOffset) && !popWorkExt )
         {
            rc = _recycleExtents( context, extent->_nextExtent, 1 ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recycle extents, rc: %d", rc ) ;
         }
         else
         {
            rc = _popFromActiveExt( context, extID, offset, 1 ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to pop records from extent, extent id: %d, "
                         "offset: %d, direction: %d, rc: %d",
                         extID, offset, 1, rc ) ;
         }
      }

      if ( popWorkExt )
      {
         rc = _attachWorkExt( context->mbID(), context->mb()->_lastExtentID ) ;
         PD_RC_CHECK( rc, PDERROR, "Attach working extent failed, rc: %d", rc ) ;
         workExtInfo->seek( expectOffset ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__SHRINKFORWARD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__SHRINKBACKWARD, "_dmsStorageDataCapped::_shrinkBackward" )
   INT32 _dmsStorageDataCapped::_shrinkBackward( dmsMBContext* context,
                                                 dmsExtentInfo* workExtInfo,
                                                 dmsExtentID extID,
                                                 dmsOffset offset )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__SHRINKBACKWARD ) ;
      dmsExtRW extRW ;
      dmsExtent *extent = NULL ;

      rc = _syncWorkExtInfo( context->mbID() ) ;
      PD_RC_CHECK( rc, PDERROR, "Sync working extent info failed, rc: %d",
                   rc ) ;

      extRW = extent2RW( extID, context->mbID() ) ;
      extRW.setNothrow( TRUE ) ;
      extent = extRW.writePtr<dmsExtent>() ;
      if ( !extent || !extent->validate( context->mbID() ))
      {
         PD_LOG( PDERROR, "Invalid extent[%d]", extID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( offset == extent->_firstRecordOffset &&
           DMS_INVALID_EXTENT != extent->_prevExtent )
      {
         // Pop all records in the target extent backwards. If it's not the last
         // extent of the collection, recycle it directly, and attach the
         // previous extent ad the working extent.
         extID = extent->_prevExtent ;
         rc = _recycleExtents( context, extID, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to recycle extents, rc: %d", rc ) ;
      }
      else
      {
         // Recycle extents which are after the target extent.
         if ( extID != context->mb()->_lastExtentID )
         {
            rc = _recycleExtents( context, extID, -1 ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recycle extents, rc: %d", rc ) ;
         }

         rc = _popFromActiveExt( context, extID, offset, -1 ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to pop records from extent, extent id: %d, "
                      "offset: %d, direction: %d, rc: %d",
                      extID, offset, -1, rc ) ;
      }
      rc = _attachWorkExt( context->mbID(), extID ) ;
      PD_RC_CHECK( rc, PDERROR, "Attach working extent failed, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__SHRINKBACKWARD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__POPRECORD, "_dmsStorageDataCapped::_popRecord" )
   INT32 _dmsStorageDataCapped::_popRecord( dmsMBContext *context,
                                            dmsExtentID extID,
                                            dmsOffset offset,
                                            INT8 direction )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__POPRECORD ) ;
      dmsExtentInfo *workExtInfo = getWorkExtInfo( context->mbID() ) ;

      SDB_ASSERT( workExtInfo, "Work extent info pointer should not be NULL" ) ;

      if ( !( context->isMBLock( EXCLUSIVE ) ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller should hold the exclusive lock, rc: %d",
                 rc ) ;
         goto error ;
      }

      if ( direction >= 0 )
      {
         rc = _shrinkForward( context, workExtInfo, extID, offset ) ;
      }
      else
      {
         rc = _shrinkBackward( context, workExtInfo, extID, offset ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Pop record by shrinking failed, extent[%d], "
                   "offset[%d], rc: %d", extID, offset, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__POPRECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__EXTRACTRECLID, "_dmsStorageDataCapped::_extractRecLID" )
   INT32 _dmsStorageDataCapped::_extractRecLID( dmsMBContext *context,
                                                INT64 logicalID,
                                                pmdEDUCB *cb,
                                                dmsExtentID &extentID,
                                                dmsOffset &offset )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__EXTRACTRECLID ) ;
      dmsRecordRW recordRW ;
      const dmsCappedRecord *record = NULL ;
      const dmsExtent *extent = NULL ;
      BOOLEAN invalid = FALSE ;
      dmsExtentID extLID = DMS_INVALID_EXTENT ;
      dmsExtentInfo* workExtInfo = getWorkExtInfo( context->mbID() ) ;

      if ( logicalID < 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid logical id[%lld], rc: %d", logicalID, rc ) ;
         goto error ;
      }

      extentID = _logicID2ExtID( context, logicalID, extent ) ;
      if ( DMS_INVALID_EXTENT == extentID )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Logical ID[%lld] is not in valid range",
                 logicalID ) ;
         goto error ;
      }

      SDB_ASSERT( extent, "extent should not be NULL" ) ;

      _recLid2ExtLidAndOffset( logicalID, extLID, offset ) ;

      // Validate the extent id and offset, to make sure they are in range and
      // at the right position.
      if ( extentID == workExtInfo->getID() )
      {
         if ( offset < workExtInfo->_firstRecordOffset ||
              offset > workExtInfo->_lastRecordOffset )
         {
            invalid = TRUE ;
         }
      }
      else
      {
         if ( offset < extent->_firstRecordOffset ||
              offset > extent->_lastRecordOffset )
         {
            invalid = TRUE ;
         }
      }
      if ( invalid )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid logical id[%lld], rc: %d",
                 logicalID, rc ) ;
         goto error ;
      }

      {
         // Check if we can access the record normally and the logical id is
         // the same.
         const dmsRecordID recordID( extentID, offset ) ;
         recordRW = record2RW( recordID, context->mbID() ) ;
         recordRW.setNothrow( TRUE ) ;
         record = recordRW.readPtr<dmsCappedRecord>() ;
         // Only when the record is normal and logical id matches, it's the
         // right one.
         if ( !record || !record->isNormal() || ( 0 == record->getRecordNo() )
              || ( logicalID != record->getLogicalID() )  )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid logical id[%lld], rc: %d",
                    logicalID, rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__EXTRACTRECLID, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // Recycle(free) the extents before(when direction is 1) or after(when
   // direction is -1) the tareget extent. The target extent is excluded.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED_RECYCLEEXTENTS, "_dmsStorageDataCapped::_recycleExtents" )
   INT32 _dmsStorageDataCapped::_recycleExtents( dmsMBContext *context,
                                                 dmsExtentID targetExtID,
                                                 INT8 direction )
   {
      // Recycle all the extents which will be freed by pop.
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED_RECYCLEEXTENTS ) ;
      dmsExtentID beginExtID = DMS_INVALID_EXTENT ;
      dmsExtentID endExtID = DMS_INVALID_EXTENT ;
      dmsExtRW extRW ;
      const dmsExtent *extent = NULL ;

      extRW = extent2RW( targetExtID, context->mbID() ) ;
      extRW.setNothrow( TRUE ) ;
      extent = extRW.readPtr<dmsExtent>() ;
      if ( !extent )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid extent: %d, rc: %d", targetExtID, rc ) ;
         goto error ;
      }

      if ( direction >= 0 )
      {
         beginExtID = context->mb()->_firstExtentID ;
         endExtID = ( DMS_INVALID_EXTENT == extent->_prevExtent ) ?
                    targetExtID : extent->_prevExtent ;
         context->mb()->_firstExtentID = targetExtID ;
      }
      else
      {
         beginExtID = ( DMS_INVALID_EXTENT == extent->_nextExtent ) ?
                      targetExtID : extent->_nextExtent ;
         endExtID = context->mb()->_lastExtentID ;
         context->mb()->_lastExtentID = targetExtID ;
      }

      // The target extent should not be recycled.
      while ( DMS_INVALID_EXTENT != beginExtID && beginExtID != targetExtID )
      {
         extRW = extent2RW( beginExtID, context->mbID() ) ;
         extRW.setNothrow( TRUE ) ;
         extent = extRW.readPtr<dmsExtent>() ;
         if ( !extent )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid extent[%d], rc: %d", beginExtID, rc ) ;
            goto error ;
         }

         rc = _recycleActiveExt( context, beginExtID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to recycle active extent, rc: %d", rc ) ;
         if ( beginExtID == endExtID )
         {
            break ;
         }

         beginExtID = extent->_nextExtent;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED_RECYCLEEXTENTS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // Get the offset of the previous record in the current extent. As there is
   // no backward link, we need to scan from the beginning.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__GETPREVRECOFFSET, "_dmsStorageDataCapped::_getPrevRecOffset" )
   INT32 _dmsStorageDataCapped::_getPrevRecOffset( UINT16 collectionID,
                                                   dmsExtentID extentID,
                                                   dmsOffset offset,
                                                   dmsOffset &prevOffset )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__GETPREVRECOFFSET ) ;

      dmsExtentInfo *workExtInfo = getWorkExtInfo( collectionID ) ;
      dmsExtRW extentRW ;
      const dmsExtent *extent = NULL ;
      dmsOffset firstOffset = DMS_INVALID_OFFSET ;
      dmsOffset lastOffset = DMS_INVALID_OFFSET ;
      dmsOffset searchOffset = DMS_INVALID_OFFSET ;
      dmsOffset prev = DMS_INVALID_OFFSET ;
      dmsRecordRW recordRW ;
      dmsCappedRecord *record = NULL ;

      prevOffset = DMS_INVALID_OFFSET ;

      if ( workExtInfo->getID() == extentID )
      {
         firstOffset = workExtInfo->_firstRecordOffset ;
         lastOffset = workExtInfo->_lastRecordOffset ;
      }
      else
      {
         extentRW = extent2RW( extentID, collectionID ) ;
         extentRW.setNothrow( TRUE ) ;
         extent = extentRW.readPtr<dmsExtent>() ;
         if ( !extent || !extent->validate( collectionID ) )
         {
            PD_LOG( PDERROR, "Invalid extent[%d]", extentID ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         firstOffset = extent->_firstRecordOffset ;
         lastOffset = extent->_lastRecordOffset ;
      }

      if ( DMS_INVALID_OFFSET == offset ||
           offset < firstOffset || offset > lastOffset )
      {
#ifdef _DEBUG
         pmdEDUCB* cb = pmdGetThreadEDUCB() ;
         if ( EDU_TYPE_REPLAGENT == cb->getType() )
         {
            SDB_ASSERT( FALSE, "Offset is out of range" ) ;
         }
#endif /* _DEBUG */

         PD_LOG( PDERROR, "Offset[%d] out of range", offset ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      searchOffset = firstOffset ;
      prev = DMS_INVALID_OFFSET ;
      while ( searchOffset < offset )
      {
         dmsRecordID rid( extentID, searchOffset ) ;
         recordRW = record2RW( rid, collectionID ) ;
         recordRW.setNothrow( TRUE ) ;
         record = recordRW.writePtr<dmsCappedRecord>() ;
         // When meet an invalid record, break;
         if ( !record || 0 == record->isNormal() )
         {
            PD_LOG( PDERROR, "Invalid record" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         // Remember the last valid record.
         prev = searchOffset ;

         searchOffset += record->getSize() ;
      }

      if ( searchOffset == offset )
      {
         prevOffset = prev ;
      }
      else
      {
         PD_LOG( PDERROR, "Invalid record offset[%d]", offset ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__GETPREVRECOFFSET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED_COUNTRECNUMANDSIZE, "_dmsStorageDataCapped::_countRecNumAndSize" )
   INT32 _dmsStorageDataCapped::_countRecNumAndSize( UINT16 collectionID,
                                                     dmsExtentID extentID,
                                                     dmsOffset beginOffset,
                                                     dmsOffset endOffset,
                                                     UINT32 &recNum,
                                                     UINT32 &totalSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED_COUNTRECNUMANDSIZE ) ;
      dmsRecordRW beginRecRW ;
      dmsRecordRW endRecRW ;
      const dmsCappedRecord *beginRec = NULL ;
      const dmsCappedRecord *endRec = NULL ;

      recNum = 0 ;
      totalSize = 0 ;

      if ( DMS_INVALID_OFFSET == beginOffset || beginOffset > endOffset )
      {
         recNum = 0 ;
         totalSize = 0 ;
         goto done ;
      }

      beginRecRW = record2RW( dmsRecordID(extentID, beginOffset),
                              collectionID ) ;
      beginRecRW.setNothrow( TRUE ) ;
      endRecRW = record2RW( dmsRecordID(extentID, endOffset), collectionID ) ;
      endRecRW.setNothrow( TRUE ) ;
      beginRec = beginRecRW.readPtr<dmsCappedRecord>() ;
      endRec = endRecRW.readPtr<dmsCappedRecord>() ;
      if ( !beginRec || !beginRec->isNormal() )
      {
         PD_LOG( PDERROR, "Invalid begin record: extent[%d], offset[%d]",
                 extentID, beginOffset ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      if ( !endRec || !endRec->isNormal() )
      {
         PD_LOG( PDERROR, "Invalid end record: extent[%d], offset[%d]",
                 extentID, endOffset ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      recNum = endRec->getRecordNo() - beginRec->getRecordNo() + 1 ;
      totalSize = endOffset - beginOffset + endRec->getSize() ;

   done:
      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACAPPED_COUNTRECNUMANDSIZE ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__UPDATEEXTENTLID, "_dmsStorageDataCapped::_updateExtentLID" )
   INT32 _dmsStorageDataCapped::_updateExtentLID( UINT16 mbID,
                                                  dmsExtentID extID,
                                                  dmsExtentID extLogicID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__UPDATEEXTENTLID ) ;
      SDB_ASSERT( DMS_INVALID_EXTENT != extID, "Extent id is invalid" ) ;
      SDB_ASSERT( DMS_INVALID_EXTENT != extLogicID,
                  "Extent logical id is invalid" ) ;

      dmsExtRW extRW = extent2RW( extID, mbID ) ;
      extRW.setNothrow( TRUE ) ;
      dmsExtent *extent = extRW.writePtr<dmsExtent>() ;
      if ( !extent || !extent->validate() )
      {
         PD_LOG( PDERROR, "Extent[ %d ] is invalid", extID ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      extent->_logicID = extLogicID ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__UPDATEEXTENTLID, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__LIMITPROCESS, "_dmsStorageDataCapped::_limitProcess" )
   INT32 _dmsStorageDataCapped::_limitProcess( dmsMBContext *context,
                                               UINT32 sizeReq,
                                               dmsExtentInfo *workExtInfo )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__LIMITPROCESS ) ;

      // Check if exceeding the Size and Max limitations.
      // If yes, return error or recycle extent, depending on the option.
      if ( _numExceedLimit( context, 1 ) )
      {
         if ( _overwriteOnExceed( context ) )
         {
            rc = _recycleOneExtent( context ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Recycle the eldest extent failed[ %d ]", rc ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Record number exceed the limit" ) ;
            rc = SDB_OSS_UP_TO_LIMIT ;
            goto error ;
         }
      }

      // If free space in current working extent is not enough, get another.
      if ( workExtInfo->_freeSpace < sizeReq )
      {
         // Before allocationg another extent, check size limitation.
         if ( !spaceEnough( context, DMS_CAP_EXTENT_SZ ) )
         {
            if ( _overwriteOnExceed( context ) )
            {
               rc = _recycleOneExtent( context ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Recycle the eldest extent failed[ %d ]", rc ) ;
            }
            else
            {
               PD_LOG( PDERROR, "Size exceed the limit" ) ;
               rc = SDB_OSS_UP_TO_LIMIT ;
               goto error ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__LIMITPROCESS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageDataCapped::popRecord( dmsMBContext *context,
                                           INT64 value,
                                           pmdEDUCB *cb,
                                           SDB_DPSCB *dpscb,
                                           INT8 direction,
                                           BOOLEAN byNumber )
   {
      return byNumber ?
             _popRecordByNumber( context, value, cb, dpscb, direction ) :
             _popRecordByLID( context, value, cb, dpscb, direction ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__POPRECORDBYLID, "_dmsStorageDataCapped::_popRecordByLID" )
   INT32 _dmsStorageDataCapped::_popRecordByLID( dmsMBContext *context,
                                                 INT64 logicalID,
                                                 pmdEDUCB *cb,
                                                 SDB_DPSCB *dpscb,
                                                 INT8 direction )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__POPRECORDBYLID ) ;
      dmsRecordID firstRID ;
      dmsExtRW extRW ;
      const dmsExtent *startExtent = NULL ;
      UINT32 logRecSize = 0 ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
      dpsMergeInfo info ;
      dmsExtentID extentID = DMS_INVALID_EXTENT ;
      dmsOffset offset = 0 ;
      dpsLogRecord &dpsRecord = info.getMergeBlock().record() ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = { 0 } ;

      SDB_ASSERT( context, "context should not be NULL" ) ;
      SDB_ASSERT( cb, "edu cb should not be NULL" ) ;

      // Calculate the target extent id and offset.
      rc = _extractRecLID( context, logicalID, cb, extentID, offset ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid LogicalID[%lld], rc: %d",
                   logicalID, rc ) ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         PD_LOG( PDERROR, "Caller must hold mb exclusive lock[%s]",
                 context->toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

#ifdef _DEBUG
      // Here we use delete access type.
      if ( !dmsAccessAndFlagCompatiblity( context->mb()->_flag,
                                          DMS_ACCESS_TYPE_DELETE ) )
      {
         PD_LOG( PDERROR, "Imcompatible collection mode: %d",
                 context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }
#endif /* _DEBUG */

      // If the collection is emplty, return directly.
      if ( 0 == _mbStatInfo[ context->mbID() ]._totalRecords )
      {
         goto done ;
      }

      if ( dpscb )
      {
         _clFullName( context->mb()->_collectionName, fullName,
                      sizeof(fullName) ) ;
         rc = dpsPop2Record( fullName, firstRID, logicalID,
                             direction, dpsRecord ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build record, rc: %d", rc ) ;

         rc = dpscb->checkSyncControl( dpsRecord.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Check sync control failed, rc: %d", rc ) ;

         logRecSize = dpsRecord.alignedLen() ;
         rc = pTransCB->reservedLogSpace( logRecSize, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to reserve log space"
                    "(length=%u), rc: %d", logRecSize, rc ) ;
            logRecSize = 0 ;
            goto error ;
         }
      }

      rc = _popRecord( context, extentID, offset, direction ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to pop records, logical id: %lld, "
                   "rc: %d", logicalID, rc ) ;

      extRW = extent2RW( context->mb()->_firstExtentID, context->mbID() ) ;
      startExtent = extRW.readPtr<dmsExtent>() ;
      if ( dpscb )
      {
         rc = _logDPS( dpscb, info, cb, context, startExtent->_logicID, FALSE,
                       DMS_FILE_DATA ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to insert record into log, rc: %d", rc ) ;
      }

   done:
      if ( 0 != logRecSize)
      {
         pTransCB->releaseLogSpace( logRecSize, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__POPRECORDBYLID, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__POPRECORDBYNUMBER, "_dmsStorageDataCapped::_popRecordByNumber" )
   INT32 _dmsStorageDataCapped::_popRecordByNumber( dmsMBContext *context,
                                                    INT64 number,
                                                    pmdEDUCB *cb,
                                                    SDB_DPSCB *dpscb,
                                                    INT8 direction )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__POPRECORDBYNUMBER ) ;
      dmsExtentInfo *extInfo = NULL ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         PD_LOG( PDERROR, "Caller must hold mb exclusive lock[%s]",
                 context->toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      while ( number-- > 0 )
      {
         INT64 recordLID = DMS_INVALID_REC_LOGICALID ;
         if ( 0 == _mbStatInfo[context->mbID()]._totalRecords )
         {
            // No more records
            goto done ;
         }

         if ( 1 == direction )
         {
            // Pop the first record in the first extent.
            const dmsExtent *extent = NULL ;
            dmsExtRW extRW = extent2RW( context->mb()->_firstExtentID,
                                        context->mbID() ) ;
            extRW.setNothrow( TRUE ) ;
            extent = extRW.readPtr<dmsExtent>() ;
            PD_CHECK( extent, SDB_SYS, error, PDERROR, "Read extent[%d] "
                                                       "failed: %d",
                      context->mb()->_firstExtentID, rc ) ;
            _extLidAndOffset2RecLid( extent->_logicID,
                                     extent->_firstRecordOffset, recordLID ) ;
         }
         else
         {
            // Pop the last record in the last extent(working extent).
            extInfo = getWorkExtInfo( context->mbID() ) ;
            _extLidAndOffset2RecLid( extInfo->_extLogicID,
                                     extInfo->_lastRecordOffset, recordLID ) ;
         }

         rc = _popRecordByLID( context, recordLID, cb, dpscb, direction ) ;
         PD_RC_CHECK( rc, PDERROR, "Pop record of logical id[%lld] failed: %d",
                      recordLID, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__POPRECORDBYNUMBER, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED_DUMPEXTOPTIONS, "_dmsStorageDataCapped::dumpExtOptions" )
   INT32 _dmsStorageDataCapped::dumpExtOptions( dmsMBContext *context,
                                                BSONObj &extOptions )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED_DUMPEXTOPTIONS ) ;
      dmsExtRW extentRW ;
      const dmsOptExtent *optExtent = NULL ;
      dmsCappedCLOptions *options = NULL ;
      UINT32 optSize = 0 ;
      BSONObjBuilder builder ;

      if ( !context->isMBLock() )
      {
         PD_RC_CHECK( rc, PDERROR, "Caller should hold mb shared lock[%s]",
                      context->toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( DMS_INVALID_EXTENT == context->mb()->_mbOptExtentID )
      {
         PD_LOG( PDERROR, "Extend option extent id is invalid for "
                 "collection[%s]", context->mb()->_collectionName ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      extentRW = extent2RW( context->mb()->_mbOptExtentID, context->mbID() ) ;
      extentRW.setNothrow( TRUE ) ;
      optExtent = extentRW.readPtr<dmsOptExtent>( 0, pageSize() ) ;
      if ( !optExtent )
      {
         PD_LOG( PDERROR, "Extend option extent is invalid for collection[%s]",
                 context->mb()->_collectionName ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = optExtent->getOption( (CHAR **)&options, &optSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Get extend options from extent failed for "
                   "collection[%s], rc: %d",
                   context->mb()->_collectionName, rc ) ;
      SDB_ASSERT( sizeof( dmsCappedCLOptions ) == optSize,
                  "Option size is not as expected" ) ;

      try
      {
         builder.append( FIELD_NAME_SIZE, options->_maxSize ) ;
         builder.append( FIELD_NAME_MAX, options->_maxRecNum ) ;
         builder.appendBool( FIELD_NAME_OVERWRITE, options->_overwrite ) ;

         extOptions = builder.obj() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Unexpected exception happened: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED_DUMPEXTOPTIONS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED_SETEXTOPTIONS, "_dmsStorageDataCapped::setExtOptions" )
   INT32 _dmsStorageDataCapped::setExtOptions ( dmsMBContext * context,
                                                const BSONObj & extOptions )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED_SETEXTOPTIONS ) ;

      SDB_ASSERT( NULL != context, "context is invalid" ) ;

      const CHAR * collection = NULL ;
      UINT16 mbID = DMS_INVALID_MBID ;
      dmsExtentID optExtentID = DMS_INVALID_EXTENT ;
      dmsExtRW optExtRW ;
      dmsCappedCLOptions options ;
      dmsOptExtent * optExtent = NULL ;

      PD_CHECK( context->isMBLock( EXCLUSIVE ), SDB_SYS, error, PDERROR,
                "Caller should hold mb exclusive lock [%s]",
                context->toString().c_str() ) ;

      collection = context->mb()->_collectionName ;
      mbID = context->mbID() ;
      optExtentID = context->mb()->_mbOptExtentID ;

      PD_CHECK( DMS_INVALID_EXTENT != optExtentID, SDB_SYS, error, PDERROR,
                "Extend option extent id is invalid for "
                "collection [%s]", collection ) ;

      rc = _parseExtendOptions( &extOptions, options ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse options, rc: %d", rc ) ;

      // Init the option page.
      optExtRW = extent2RW( context->mb()->_mbOptExtentID, context->mbID() ) ;
      optExtRW.setNothrow( TRUE ) ;
      optExtent = optExtRW.writePtr<dmsOptExtent>( 0, pageSize() ) ;
      PD_CHECK( NULL != optExtent && optExtent->_mbID == context->mbID(),
                SDB_SYS, error, PDERROR,
                "Invalid option extent[%d]", optExtent ) ;

      optExtent->setOption( (const CHAR *)&options,
                            sizeof( dmsCappedCLOptions )) ;

      // Flush immediately to avoid corruption of this page in crash.
      flushPages( context->mb()->_mbOptExtentID, pageSize(), isSyncDeep() ) ;

      rc = optExtent->getOption( (CHAR **)&_options[ mbID ], NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Get collection extend option from extent "
                   "failed, rc: %d", rc ) ;
      SDB_ASSERT( NULL != _options[ mbID ], "Option pointer should not be NULL" ) ;

   done :
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED_SETEXTOPTIONS, rc ) ;
      return rc ;

   error :
      goto done ;
   }
}
