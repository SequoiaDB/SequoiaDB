/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__ONRESTORE, "_dmsStorageDataCapped::_onRestore" )
   void _dmsStorageDataCapped::_onRestore()
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__ONRESTORE ) ;
      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS; ++i )
      {
         if ( DMS_IS_MB_INUSE ( _dmsMME->_mbList[i]._flag ) &&
              ( DMS_INVALID_EXTENT != _dmsMME->_mbList[i]._lastExtentID ) )
         {
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
      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS; ++i )
      {
         if ( DMS_IS_MB_INUSE( _dmsMME->_mbList[i]._flag ) )
         {
            dmsExtentInfo *workExtInfo = getWorkExtInfo( i ) ;
            dmsExtentID extentID = workExtInfo->_id ;
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

      optExtRW = extent2RW( extOptExtent, collectionID ) ;
      optExtRW.setNothrow( TRUE ) ;
      optExtent = optExtRW.writePtr<dmsOptExtent>( 0, extentSize <<
                                                      pageSizeSquareRoot() ) ;
      PD_CHECK( optExtent, SDB_SYS, error, PDERROR,
                "Invalid option extent[%d]", optExtent ) ;
      optExtent->init( extentSize, collectionID ) ;
      optExtent->setOption( (const CHAR *)&options,
                            sizeof( dmsCappedCLOptions )) ;

      flushPages( extOptExtent, extentSize, isSyncDeep() ) ;

      rc = optExtent->getOption( (CHAR **)&_options[collectionID], NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Get collection extend option from extent "
                   "failed, rc: %d", rc ) ;
      SDB_ASSERT( _options[collectionID],
                  "Option pointer should not be NULL" ) ;

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

         INT32 idSize = 0 ;
         UINT32 totalSize = logicalIDEle.size() + record.objsize() ;
         BSONElement ele = record.getField( DMS_ID_KEY_NAME ) ;
         if ( EOO != ele.type() )
         {
            idSize = ele.size() ;
            totalSize -= idSize ;
         }

         rc = cb->allocBuff( totalSize, &mergedData ) ;
         PD_RC_CHECK( rc, PDERROR, "Allocate memory[size: %u] failed, rc: %d",
                      totalSize, rc ) ;

         *(UINT32*)mergedData = totalSize ;
         writePos = mergedData + sizeof(UINT32) ;
         ossMemcpy( writePos, logicalIDEle.rawdata(), logicalIDEle.size() ) ;
         writePos += logicalIDEle.size() ;
         if ( 0 == idSize || ( ele.value() == mergedData + sizeof(UINT32) ) )
         {
            ossMemcpy( writePos,
                       record.objdata() + sizeof(UINT32) + idSize,
                       record.objsize() - sizeof(UINT32) - idSize ) ;
         }
         else
         {
            const CHAR *copyPos = record.objdata() + sizeof(UINT32) ;
            INT32 copyLen = ele.rawdata() - record.objdata() - sizeof(UINT32) ;
            ossMemcpy( writePos, copyPos, copyLen ) ;
            writePos += copyLen ;
            copyPos += copyLen + idSize ;
            INT32 remainLen = record.objsize() -
                              ( copyPos - record.objdata() ) ;
            SDB_ASSERT( remainLen >= 1,
                        "Remain length should be greater than 0" ) ;
            if ( remainLen > 0 )
            {
               ossMemcpy( writePos, copyPos, remainLen ) ;
            }
         }
         recordData.setData( mergedData, totalSize, FALSE, TRUE ) ;
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
      if ( !recordData.isCompressed() )
      {
         size += sizeof( LogicalIDToInsert ) ;
      }

      size += DMS_RECORD_METADATA_SZ ;
      size = OSS_MIN( DMS_RECORD_MAX_SZ, ossAlignX ( size, 4 ) ) ;
      PD_TRACE2 ( SDB__DMSSTORAGEDATACAPPED__FINALRECORDSIZE,
                  PD_PACK_STRING ( "size after align" ),
                  PD_PACK_UINT ( size ) ) ;
      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACAPPED__FINALRECORDSIZE ) ;
   }

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

      if ( DMS_INVALID_EXTENT == workExtInfo->getID() )
      {
         SDB_ASSERT( DMS_INVALID_EXTENT == context->mb()->_firstExtentID
                     && DMS_INVALID_EXTENT == context->mb()->_lastExtentID,
                     "The first and last extents should be invalid" ) ;
         rc = _allocateExtent( context, DMS_CAP_EXTENT_PAGE_NUM,
                               TRUE, FALSE, &extID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to allocate extent, rc: %d", rc ) ;


         if ( DMS_INVALID_EXTENT == workExtInfo->getID() ||
              workExtInfo->_freeSpace < size )
         {
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
            rc = _updateExtentLID( context->mbID(), extID, extLogicalID ) ;
            PD_RC_CHECK( rc, PDERROR, "Update extent logical id to %d "
                         "failed[ %d ]", extLogicalID, rc ) ;
            _mbStatInfo[context->mbID()]._totalDataFreeSpace -=
               offset - DMS_EXTENT_METADATA_SZ ;
         }

         if ( DMS_INVALID_EXTENT == workExtInfo->getID() ||
              workExtInfo->_freeSpace < size )
         {
            rc = _attachNextExt( context, workExtInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Switch working extent failed, "
                         "rc: %d", rc ) ;
            workExtInfo->seek( offset ) ;
         }
      }
      else
      {
         if ( extLogicalID == workExtInfo->_extLogicID )
         {
            if ( offset != workExtInfo->getNextRecOffset() )
            {
               PD_LOG( PDERROR, "Extent logical id[ %d ] from position[ %lld ] "
                       "is invalid", extLogicalID, position ) ;
               rc = SDB_INVALIDARG ;
               goto done ;
            }
         }
         else if ( extLogicalID == ( workExtInfo->_extLogicID + 1 ) )
         {
            INT64 logicalID = DMS_INVALID_REC_LOGICALID ;
            _extLidAndOffset2RecLid( extLogicalID, DMS_EXTENT_METADATA_SZ,
                                     logicalID ) ;
            if ( position != logicalID )
            {
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
   INT32 _dmsStorageDataCapped::extractData( dmsMBContext *mbContext,
                                             dmsRecordRW &recordRW,
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
                          FALSE, TRUE ) ;

      if ( pRecord->isCompressed() )
      {
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
         if ( unCompressDataLen != *(INT32*)uncompressData )
         {
            PD_LOG( PDERROR, "Uncompress data length[%d] does not match "
                    "real length[%d]", unCompressDataLen,
                    *(INT32*)uncompressData ) ;
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__ONALLOCSPACEREADY, "_dmsStorageDataCapped::_onAllocSpaceReady" )
   void _dmsStorageDataCapped::_onAllocSpaceReady( dmsContext *context,
                                                    BOOLEAN &doit )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__ONALLOCSPACEREADY ) ;
      dmsExtentInfo *extInfo = getWorkExtInfo( context->mbID() ) ;
      dmsExtentID workExtID = extInfo->getID() ;

      if ( DMS_INVALID_EXTENT == workExtID )
      {
         doit = TRUE ;
      }
      else
      {
         dmsExtRW extRW = extent2RW( workExtID, context->mbID() );
         extRW.setNothrow( TRUE ) ;
         const dmsExtent *extent = extRW.readPtr<dmsExtent>() ;
         SDB_ASSERT( extent, "Extent is invalid" ) ;
         doit = ( DMS_INVALID_EXTENT == extent->_nextExtent ) ? TRUE : FALSE ;
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

      currExtLID = extent->_logicID ;
      extent->init( DMS_CAP_EXTENT_PAGE_NUM, mbID, DMS_CAP_EXTENT_SZ ) ;
      extent->_logicID = currExtLID + 1 ;

      _mbStatInfo[mbID]._totalRecords -= extInfo->_recCount ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__RECYCLEEXTENT, "_dmsStorageDataCapped::_recycleExtent" )
   INT32 _dmsStorageDataCapped::_recycleOneExtent( dmsMBContext *context )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__RECYCLEEXTENT ) ;
      dmsExtentID extentID = DMS_INVALID_EXTENT ;
      dmsExtentInfo *workExtInfo = getWorkExtInfo( context->mbID() ) ;

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
      pRecord->setNormal() ;
      pRecord->resetAttr() ;
      pRecord->setSize( recordSize ) ;
      pRecord->setRecordNo( workExtInfo->currentRecNo() + 1 ) ;
      {
         LogicalIDToInsertEle ele( (CHAR *)( recordData.data() +
                                             sizeof(UINT32) ) ) ;
         INT64 *lidPtr = (INT64 *)ele.value() ;
         *lidPtr = workExtInfo->getRecordLogicID() ;
         pRecord->setLogicalID( *lidPtr ) ;
      }

      pRecord->setData( recordData ) ;

      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;

      _updateStatInfo( context, recordSize, recordData ) ;

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
                                                      pmdEDUCB *cb )
   {
      SDB_ASSERT( FALSE, "Should not be here" ) ;
      return SDB_OPERATION_INCOMPATIBLE ;
   }

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

      if ( recordRW.getRecordID() != lastRID )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Only allowed to delete the last record in "
                 "capped collection, rc: %d", rc ) ;
         goto error ;
      }

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

      popWorkExt = ( workExtInfo->getID() == extID ) ? TRUE : FALSE ;
      if ( popWorkExt )
      {
         rc = _syncWorkExtInfo( context->mbID() ) ;
         PD_RC_CHECK( rc, PDERROR, "Sync working extent info failed, rc: %d",
                      rc ) ;
         expectOffset = workExtInfo->getNextRecOffset() ;
      }

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

      rc = _syncWorkExtInfo( context->mbID() ) ;
      PD_RC_CHECK( rc, PDERROR, "Sync working extent info failed, rc: %d",
                   rc ) ;

      if ( extID != context->mb()->_lastExtentID )
      {
         rc = _recycleExtents( context, extID, -1 ) ;
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

         rc = _popFromActiveExt( context, extID, offset, -1 ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to pop records from extent, extent id: %d, "
                      "offset: %d, direction: %d, rc: %d",
                      extID, offset, -1, rc ) ;
         rc = _attachWorkExt( context->mbID(), extID ) ;
         PD_RC_CHECK( rc, PDERROR, "Attach working extent failed, rc: %d",
                      rc ) ;
         workExtInfo->seek( offset ) ;
      }

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
      dmsRecordID recordID ;
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
         const dmsRecordID recordID( extentID, offset ) ;
         recordRW = record2RW( recordID, context->mbID() ) ;
         recordRW.setNothrow( TRUE ) ;
         record = recordRW.readPtr<dmsCappedRecord>() ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED_RECYCLEEXTENTS, "_dmsStorageDataCapped::_recycleExtents" )
   INT32 _dmsStorageDataCapped::_recycleExtents( dmsMBContext *context,
                                                 dmsExtentID targetExtID,
                                                 INT8 direction )
   {
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

   INT32 _dmsStorageDataCapped::_getPrevRecOffset( UINT16 collectionID,
                                                  dmsExtentID extentID,
                                                  dmsOffset offset,
                                                  dmsOffset &prevOffset )
   {
      INT32 rc = SDB_OK ;

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
         if ( !record || 0 == record->isNormal() )
         {
            PD_LOG( PDERROR, "Invalid record" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
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

      if ( workExtInfo->_freeSpace < sizeReq )
      {
         if ( _sizeExceedLimit( context, DMS_CAP_EXTENT_SZ ) )
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED_POPRECORD, "_dmsStorageDataCapped::popRecord" )
   INT32 _dmsStorageDataCapped::popRecord( dmsMBContext *context,
                                           INT64 logicalID,
                                           pmdEDUCB *cb,
                                           SDB_DPSCB *dpscb,
                                           INT8 direction )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED_POPRECORD ) ;
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
      if ( !dmsAccessAndFlagCompatiblity( context->mb()->_flag,
                                          DMS_ACCESS_TYPE_DELETE ) )
      {
         PD_LOG( PDERROR, "Imcompatible collection mode: %d",
                 context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }
#endif /* _DEBUG */

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
         info.enableTrans() ;
         rc = _logDPS( dpscb, info, cb, context, startExtent->_logicID, FALSE,
                       DMS_FILE_DATA ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to insert record into log, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED_POPRECORD, rc ) ;
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
}

