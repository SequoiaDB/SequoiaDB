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

   Source File Name = dmsStorageData.cpp

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

#include "dmsStorageData.hpp"
#include "dmsStorageIndex.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmd.hpp"
#include "mthModifier.hpp"
#include "dpsOp2Record.hpp"

namespace engine
{
   _dmsStorageData::_dmsStorageData( const CHAR *pSuFileName,
                                     dmsStorageInfo *pInfo,
                                     _IDmsEventHolder *pEventHolder )
   : _dmsStorageDataCommon( pSuFileName, pInfo, pEventHolder )
   {
   }

   _dmsStorageData::~_dmsStorageData()
   {
   }

   INT32 _dmsStorageData::_prepareAddCollection( const BSONObj *extOption,
                                                 dmsExtentID &extOptExtent,
                                                 UINT16 &extentPageNum )
   {
      return SDB_OK ;
   }

   INT32 _dmsStorageData::_onAddCollection( const BSONObj *extOption,
                                            dmsExtentID extOptExtent,
                                            UINT32 extentSize,
                                            UINT16 collectionID )
   {
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__ONALLOCEXTENT, "_dmsStorageData::_onAllocExtent" )
   void _dmsStorageData::_onAllocExtent( dmsMBContext *context,
                                         dmsExtent *extAddr,
                                         SINT32 extentID )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__ONALLOCEXTENT ) ;
      SDB_ASSERT( context, "Context should not be NULL" ) ;
      SDB_ASSERT( extAddr, "Extent address should not be NULL" ) ;

      _mapExtent2DelList( context->mb(), extAddr, extentID ) ;

      PD_TRACE_EXIT( SDB__DMSSTORAGEDATA__ONALLOCEXTENT ) ;
   }

   INT32 _dmsStorageData::_prepareInsertData( const BSONObj &record,
                                              BOOLEAN mustOID,
                                              pmdEDUCB *cb,
                                              dmsRecordData &recordData,
                                              BOOLEAN &memReallocate,
                                              INT64 position )
   {
      INT32 rc = SDB_OK ;
      IDToInsert oid ;
      idToInsertEle oidEle((CHAR*)(&oid)) ;
      CHAR *pMergedData = NULL ;

      try
      {
         recordData.setData( record.objdata(), record.objsize(),
                             FALSE, TRUE ) ;
         BSONElement ele = record.getField( DMS_ID_KEY_NAME ) ;
         const CHAR *pCheckErr = "" ;
         if ( !dmsIsRecordIDValid( ele, TRUE, &pCheckErr ) )
         {
            PD_LOG( PDERROR, "Record[%s] _id is error: %s",
                    record.toString().c_str(), pCheckErr ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( mustOID && ele.eoo() )
         {
            oid._oid.init() ;
            rc = cb->allocBuff( oidEle.size() + record.objsize(),
                                &pMergedData ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Alloc memory[size:%u] failed, rc: %d",
                       oidEle.size() + record.objsize(), rc ) ;
               goto error ;
            }
            *(UINT32*)pMergedData = oidEle.size() + record.objsize() ;
            ossMemcpy( pMergedData + sizeof(UINT32), oidEle.rawdata(),
                       oidEle.size() ) ;
            ossMemcpy( pMergedData + sizeof(UINT32) + oidEle.size(),
                       record.objdata() + sizeof(UINT32),
                       record.objsize() - sizeof(UINT32) ) ;
            recordData.setData( pMergedData,
                                oidEle.size() + record.objsize(),
                                FALSE, TRUE ) ;
            memReallocate = TRUE ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__ALLOCRECORDSPACE, "_dmsStorageData::_allocRecordSpace" )
   INT32 _dmsStorageData::_allocRecordSpace( dmsMBContext *context,
                                             UINT32 size,
                                             dmsRecordID &foundRID,
                                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__ALLOCRECORDSPACE ) ;

      rc = _reserveFromDeleteList( context, size, foundRID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Reserve delete record failed, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA__ALLOCRECORDSPACE, rc ) ;
      return rc ;
   error:
      goto done;
   }

   INT32 _dmsStorageData::_allocRecordSpaceByPos( dmsMBContext *context,
                                                  UINT32 size,
                                                  INT64 position,
                                                  dmsRecordID &foundRID,
                                                  pmdEDUCB *cb )
   {
      return SDB_OPERATION_INCOMPATIBLE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__EXTENTUPDATERECORD, "_dmsStorageData::_extentUpdatedRecord" )
   INT32 _dmsStorageData::_extentUpdatedRecord( dmsMBContext *context,
                                                dmsExtRW &extRW,
                                                dmsRecordRW &recordRW,
                                                const dmsRecordData &recordData,
                                                const BSONObj &newObj,
                                                _pmdEDUCB *cb )
   {
      INT32 rc                     = SDB_OK ;
      UINT32 dmsRecordSize         = 0 ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__EXTENTUPDATERECORD ) ;
      monAppCB * pMonAppCB         = cb ? cb->getMonAppCB() : NULL ;
      dmsCompressorEntry *compressorEntry = &_compressorEntry[context->mbID()] ;
      const dmsExtent *pExtent     = NULL ;
      dmsRecord *pRecord           = NULL ;

      dmsRecordID ovfRID ;
      dmsExtRW ovfExtRW ;
      dmsRecordRW ovfRW ;
      dmsRecord *pOvfRecord        = NULL ;
      dmsRecordData newRecordData ;

      BOOLEAN rollbackIndex        = FALSE ;

      SDB_ASSERT ( !recordData.isEmpty(), "recordData can't be empty" ) ;

      if ( newObj.objsize() + DMS_RECORD_METADATA_SZ > DMS_RECORD_USER_MAX_SZ )
      {
         PD_LOG ( PDERROR, "record is too big: %d", newObj.objsize() ) ;
         rc = SDB_DMS_RECORD_TOO_BIG ;
         goto error ;
      }

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold exclusive lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      try
      {
         pExtent = extRW.readPtr<dmsExtent>() ;
         pRecord = recordRW.writePtr( 0 ) ;

         newRecordData.setData( newObj.objdata(), newObj.objsize(),
                                FALSE, TRUE ) ;
         dmsRecordSize = newRecordData.len() ;

         if ( compressorEntry->ready() )
         {
            INT32 compressedDataSize     = 0 ;
            const CHAR *compressedData   = NULL ;
            UINT8 compressRatio          = 0 ;
            rc = dmsCompress( cb, compressorEntry,
                              newObj, NULL, 0,
                              &compressedData, &compressedDataSize,
                              compressRatio ) ;
            if ( SDB_OK == rc &&
                 compressedDataSize + sizeof(UINT32) < newRecordData.orgLen() &&
                 compressRatio < DMS_COMPRESS_RATIO_THRESHOLD )
            {
               dmsRecordSize = compressedDataSize + sizeof(UINT32) ;
               PD_TRACE2 ( SDB__DMSSTORAGEDATA__EXTENTUPDATERECORD,
                           PD_PACK_STRING ( "size after compress" ),
                           PD_PACK_UINT ( dmsRecordSize ) ) ;

               newRecordData.setData( compressedData, compressedDataSize,
                                      TRUE, FALSE ) ;
            }
         }

         dmsRecordSize += DMS_RECORD_METADATA_SZ ;
         {
            BSONObj oriObj( recordData.data() ) ;
            BSONObj newObj( newRecordData.orgData() ) ;
            rc = _pIdxSU->indexesUpdate( context, pExtent->_logicID,
                                         oriObj, newObj,
                                         recordRW.getRecordID(),
                                         cb, FALSE ) ;
            rollbackIndex = TRUE ;
            if ( rc )
            {
               PD_LOG ( PDWARNING, "Failed to update index, rc: %d", rc ) ;
               goto error ;
            }
         }

         if ( pRecord->isOvf() )
         {
            ovfRID = pRecord->getOvfRID() ;
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_READ, 1 ) ;
            ovfExtRW = extent2RW( ovfRID._extent, context->mbID() ) ;
            ovfRW = record2RW( ovfRID, context->mbID() ) ;
            pOvfRecord = ovfRW.writePtr( 0 ) ;
         }

         if ( dmsRecordSize <= pRecord->getSize() )
         {
            pRecord->setData( newRecordData ) ;
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;

            if ( ovfRID.isValid() )
            {
               _extentRemoveRecord( context, ovfExtRW, ovfRW, cb, FALSE ) ;
               pRecord->setNormal() ;
            }
            context->mbStat()->_totalDataLen -= recordData.orgLen() ;
            context->mbStat()->_totalOrgDataLen -= recordData.len() ;
            context->mbStat()->_totalDataLen += newRecordData.len() ;
            context->mbStat()->_totalOrgDataLen += newRecordData.orgLen() ;
            goto done ;
         }
         else if ( pOvfRecord && dmsRecordSize <= pOvfRecord->getSize() )
         {
            pOvfRecord->setData( newRecordData ) ;
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;
            context->mbStat()->_totalDataLen -= recordData.orgLen() ;
            context->mbStat()->_totalOrgDataLen -= recordData.len() ;
            context->mbStat()->_totalDataLen += newRecordData.len() ;
            context->mbStat()->_totalOrgDataLen += newRecordData.orgLen() ;
            goto done ;
         }
         else
         {
            dmsRecordID foundDeletedID ;
            dmsExtRW newExtRW ;
            dmsRecordRW newRecordRW ;
            const dmsExtent *pNewExtent = NULL ;
            dmsRecord *pNewRecord = NULL ;

            dmsRecordSize -= DMS_RECORD_METADATA_SZ ;
            _overflowSize( dmsRecordSize ) ;
            dmsRecordSize += DMS_RECORD_METADATA_SZ ;
            dmsRecordSize = OSS_MIN( DMS_RECORD_MAX_SZ,
                                     ossAlignX ( dmsRecordSize, 4 ) ) ;

            rc = _reserveFromDeleteList ( context, dmsRecordSize,
                                          foundDeletedID, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to reserve delete record, rc: %d", rc ) ;
               goto error ;
            }
            newExtRW = extent2RW( foundDeletedID._extent, context->mbID() ) ;
            newRecordRW = record2RW( foundDeletedID, context->mbID() ) ;
            pNewExtent = newExtRW.readPtr<dmsExtent>() ;
            pNewRecord = newRecordRW.writePtr() ;

            if ( !pNewExtent->validate( context->mbID() ) )
            {
               PD_LOG ( PDERROR, "Invalid extent[%d] is detected",
                        foundDeletedID._extent ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            rc = _extentInsertRecord ( context, newExtRW, newRecordRW,
                                       newRecordData, dmsRecordSize,
                                       cb, FALSE ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to append record due to %d", rc ) ;
               goto error ;
            }
            pNewRecord->setOvt() ;
            pRecord->setOvf() ;
            pRecord->setOvfRID( foundDeletedID ) ;
            if ( ovfRID.isValid() )
            {
               _extentRemoveRecord( context, ovfExtRW, ovfRW, cb, FALSE ) ;
            }

            context->mbStat()->_totalDataLen -= recordData.orgLen() ;
            context->mbStat()->_totalOrgDataLen -= recordData.len() ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__EXTENTUPDATERECORD, rc ) ;
      return rc ;
   error :
      if( rollbackIndex )
      {
         BSONObj oriObj( recordData.data() ) ;
         BSONObj newObj( newRecordData.orgData() ) ;
         INT32 rc1 = _pIdxSU->indexesUpdate( context, pExtent->_logicID,
                                             newObj, oriObj,
                                             recordRW.getRecordID(),
                                             cb, TRUE ) ;
         if ( rc1 )
         {
            PD_LOG ( PDERROR, "Failed to rollback update due to rc %d", rc1 ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__RESERVEFROMDELETELIST, "_dmsStorageData::_reserveFromDeleteList" )
   INT32 _dmsStorageData::_reserveFromDeleteList( dmsMBContext *context,
                                                  UINT32 requiredSize,
                                                  dmsRecordID &resultID,
                                                  pmdEDUCB * cb )
   {
      INT32 rc                      = SDB_OK ;
      UINT32 dmsRecordSizeTemp      = 0 ;
      UINT8  deleteRecordSlot       = 0 ;
      const static INT32 s_maxSearch = 3 ;

      INT32  j                      = 0 ;
      INT32  i                      = 0 ;
      dmsRecordID foundDeletedID  ;
      dmsRecordRW delRecordRW ;
      const dmsDeletedRecord* pRead = NULL ;
      dpsTransCB *pTransCB          = pmdGetKRCB()->getTransCB() ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__RESERVEFROMDELETELIST ) ;
      PD_TRACE1 ( SDB__DMSSTORAGEDATA__RESERVEFROMDELETELIST,
                  PD_PACK_UINT ( requiredSize ) ) ;
      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

   retry:
      dmsRecordSizeTemp = ( requiredSize-1 ) >> 5 ;
      deleteRecordSlot  = 0 ;
      while ( dmsRecordSizeTemp != 0 )
      {
         deleteRecordSlot ++ ;
         dmsRecordSizeTemp = dmsRecordSizeTemp >> 1 ;
      }
      SDB_ASSERT( deleteRecordSlot < dmsMB::_max, "Invalid record size" ) ;

      if ( deleteRecordSlot >= dmsMB::_max )
      {
         PD_LOG( PDERROR, "Invalid record size: %u", requiredSize ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = SDB_DMS_NOSPC ;
      try
      {
         for ( j = deleteRecordSlot ; j < dmsMB::_max ; ++j )
         {
            dmsRecordRW preRW ;
            foundDeletedID = _dmsMME->_mbList[context->mbID()]._deleteList[j] ;
            for ( i = 0 ; i < s_maxSearch ; ++i )
            {
               if ( foundDeletedID.isNull() )
               {
                  break ;
               }
               delRecordRW = record2RW( foundDeletedID, context->mbID() ) ;
               pRead = delRecordRW.readPtr<dmsDeletedRecord>() ;

               if( pRead->isDeleted() && pRead->getSize() >= requiredSize )
               {
                  if ( SDB_OK == pTransCB->transLockTestX( cb, _logicalCSID,
                                                           context->mbID(),
                                                           &foundDeletedID ) )
                  {
                     if ( preRW.isEmpty() )
                     {
                        context->mb()->_deleteList[j] = pRead->getNextRID() ;
                     }
                     else
                     {
                        dmsDeletedRecord *preWrite =
                           preRW.writePtr<dmsDeletedRecord>() ;
                        preWrite->setNextRID( pRead->getNextRID() ) ;
                     }

                     dmsExtRW rw = extent2RW( foundDeletedID._extent,
                                              context->mbID() ) ;
                     dmsExtent *pExtent = rw.writePtr<dmsExtent>() ;
                     pExtent->_freeSpace -= pRead->getSize() ;
                     context->mbStat()->_totalDataFreeSpace -= pRead->getSize() ;

                     resultID = foundDeletedID ;
                     rc = SDB_OK ;
                     goto done ;
                  }
                  else
                  {
                     --i ;
                  }
               }

               preRW = delRecordRW ;
               foundDeletedID = pRead->getNextRID() ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

      {
         UINT32 expandSize = requiredSize << DMS_RECORDS_PER_EXTENT_SQUARE ;
         if ( expandSize > DMS_BEST_UP_EXTENT_SZ )
         {
            expandSize = requiredSize < DMS_BEST_UP_EXTENT_SZ ?
                         DMS_BEST_UP_EXTENT_SZ : requiredSize ;
         }
         UINT32 reqPages = ( expandSize + DMS_EXTENT_METADATA_SZ +
                             pageSize() - 1 ) >> pageSizeSquareRoot() ;
         if ( reqPages > segmentPages() )
         {
            reqPages = segmentPages() ;
         }
         if ( reqPages < 1 )
         {
            reqPages = 1 ;
         }

         rc = _allocateExtent( context, reqPages, TRUE, FALSE, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Unable to allocate %d pages extent to the "
                      "collection, rc: %d", reqPages, rc ) ;
         goto retry ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__RESERVEFROMDELETELIST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD, "_dmsStorageData::_saveDeletedRecord" )
   INT32 _dmsStorageData::_saveDeletedRecord( dmsMB *mb,
                                              const dmsRecordID &rid,
                                              INT32 recordSize,
                                              dmsExtent *extAddr,
                                              dmsDeletedRecord *pRecord )
   {
      UINT8 deleteRecordSlot = 0 ;

      SDB_ASSERT( extAddr && pRecord, "NULL Pointer" ) ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD ) ;
      PD_TRACE6 ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD,
                  PD_PACK_STRING ( "offset" ),
                  PD_PACK_INT ( rid._offset ),
                  PD_PACK_STRING ( "recordSize" ),
                  PD_PACK_INT ( recordSize ),
                  PD_PACK_STRING ( "extentID" ),
                  PD_PACK_INT ( rid._extent ) ) ;

      pRecord->setDeleted() ;
      if ( recordSize > 0 )
      {
         pRecord->setSize( recordSize ) ;
      }
      else
      {
         recordSize = pRecord->getSize() ;
      }
      pRecord->setMyOffset( rid._offset ) ;

      extAddr->_freeSpace += recordSize ;
      _mbStatInfo[mb->_blockID]._totalDataFreeSpace += recordSize ;

      recordSize = ( recordSize - 1 ) >> 5 ;

      while ( (recordSize) != 0 )
      {
         deleteRecordSlot ++ ;
         recordSize = ( recordSize >> 1 ) ;
      }

      SDB_ASSERT ( deleteRecordSlot < dmsMB::_max, "Invalid record size" ) ;

      pRecord->setNextRID( mb->_deleteList [ deleteRecordSlot ] ) ;
      mb->_deleteList[ deleteRecordSlot ] = rid ;
      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD1, "_dmsStorageData::_saveDeletedRecord" )
   INT32 _dmsStorageData::_saveDeletedRecord( dmsMB * mb,
                                              const dmsRecordID &recordID,
                                              INT32 recordSize )
   {
      INT32 rc = SDB_OK ;
      dmsExtRW rw ;
      dmsRecordRW recordRW ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD1 ) ;
      PD_TRACE2 ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD1,
                  PD_PACK_INT ( recordID._extent ),
                  PD_PACK_INT ( recordID._offset ) ) ;
      if ( recordID.isNull() )
      {
         rc = SDB_INVALIDARG ;
         goto done ;
      }

      rw = extent2RW( recordID._extent, mb->_blockID ) ;
      recordRW = record2RW( recordID, mb->_blockID ) ;

      try
      {
         dmsExtent *pExtent = rw.writePtr<dmsExtent>() ;
         dmsDeletedRecord* pRecord = recordRW.writePtr<dmsDeletedRecord>() ;
         rc = _saveDeletedRecord ( mb, recordID, recordSize,
                                   pExtent, pRecord ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD1, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__MAPEXTENT2DELLIST, "_dmsStorageData::_mapExtent2DelList" )
   void _dmsStorageData::_mapExtent2DelList( dmsMB *mb, dmsExtent *extAddr,
                                             SINT32 extentID )
   {
      INT32 extentSize         = 0 ;
      INT32 extentUseableSpace = 0 ;
      INT32 deleteRecordSize   = 0 ;
      dmsOffset recordOffset   = 0 ;
      INT32 curUseableSpace    = 0 ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__MAPEXTENT2DELLIST ) ;

      if ( (UINT32)extAddr->_freeSpace < DMS_MIN_RECORD_SZ )
      {
         if ( extAddr->_freeSpace != 0 )
         {
            PD_LOG( PDINFO, "Collection[%s, mbID: %d]'s extent[%d] free "
                    "space[%d] is less than min record size[%d]",
                    mb->_collectionName, mb->_blockID, extentID,
                    extAddr->_freeSpace, DMS_MIN_RECORD_SZ ) ;
         }
         goto done ;
      }

      extentSize          = extAddr->_blockSize << pageSizeSquareRoot() ;
      extentUseableSpace  = extAddr->_freeSpace ;
      extAddr->_freeSpace = 0 ;

      deleteRecordSize    = OSS_MIN ( extentUseableSpace,
                                      DMS_RECORD_MAX_SZ ) ;
      recordOffset        = extentSize - extentUseableSpace ;
      curUseableSpace     = extentUseableSpace ;

      while ( curUseableSpace - deleteRecordSize >=
              (INT32)DMS_MIN_DELETEDRECORD_SZ )
      {
         dmsRecordID rid( extentID, recordOffset ) ;
         dmsRecordRW rRW = record2RW( rid, mb->_blockID ) ;
         _saveDeletedRecord( mb, rid, deleteRecordSize,
                             extAddr, rRW.writePtr<dmsDeletedRecord>() ) ;
         curUseableSpace -= deleteRecordSize ;
         recordOffset += deleteRecordSize ;
      }
      if ( curUseableSpace > deleteRecordSize )
      {
         dmsRecordID rid( extentID, recordOffset ) ;
         dmsRecordRW rRW = record2RW( rid, mb->_blockID ) ;
         _saveDeletedRecord( mb, rid, DMS_PAGE_SIZE4K,
                             extAddr, rRW.writePtr<dmsDeletedRecord>() ) ;
         curUseableSpace -= DMS_PAGE_SIZE4K ;
         recordOffset += DMS_PAGE_SIZE4K ;
      }
      if ( curUseableSpace > 0 )
      {
         dmsRecordID rid( extentID, recordOffset ) ;
         dmsRecordRW rRW = record2RW( rid, mb->_blockID ) ;
         _saveDeletedRecord( mb, rid, curUseableSpace,
                             extAddr, rRW.writePtr<dmsDeletedRecord>() ) ;
      }

      SDB_ASSERT( extentUseableSpace == extAddr->_freeSpace,
                  "Extent[%d] free space invalid" ) ;
   done :
      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATA__MAPEXTENT2DELLIST ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__EXTENTINSERTRECORD, "_dmsStorageData::_extentInsertRecord" )
   INT32 _dmsStorageData::_extentInsertRecord( dmsMBContext *context,
                                               dmsExtRW &extRW,
                                               dmsRecordRW &recordRW,
                                               const dmsRecordData &recordData,
                                               UINT32 needRecordSize,
                                               _pmdEDUCB *cb,
                                               BOOLEAN isInsert )
   {
      INT32 rc                         = SDB_OK ;
      monAppCB * pMonAppCB             = cb ? cb->getMonAppCB() : NULL ;
      dmsRecord* pRecord               = NULL ;
      dmsOffset  myOffset              = DMS_INVALID_OFFSET ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__EXTENTINSERTRECORD ) ;
      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      pRecord = recordRW.writePtr( needRecordSize ) ;
      myOffset = pRecord->getMyOffset() ;
      if ( pRecord->getSize() < needRecordSize )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( !recordData.isCompressed()
                && recordData.len() < DMS_MIN_RECORD_DATA_SZ )
      {
         PD_LOG( PDERROR, "Bson obj size[%d] is invalid",
                 recordData.len() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      pRecord->setNormal() ;
      pRecord->resetAttr() ;

      if ( pRecord->getSize() - needRecordSize > DMS_MIN_RECORD_SZ )
      {
         dmsOffset newOffset = myOffset + needRecordSize ;
         INT32 newSize = pRecord->getSize() - needRecordSize ;
         dmsRecordID newRid = recordRW.getRecordID() ;
         newRid._offset = newOffset ;
         rc = _saveDeletedRecord( context->mb(), newRid, newSize ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to save deleted record, rc: %d", rc ) ;
            goto error ;
         }
         pRecord->setSize( needRecordSize ) ;
      }

      pRecord->setData( recordData ) ;

      pRecord->setNextOffset( DMS_INVALID_OFFSET ) ;
      pRecord->setPrevOffset( DMS_INVALID_OFFSET ) ;

      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;

      if ( isInsert )
      {
         dmsExtent *extent       = extRW.writePtr<dmsExtent>() ;
         dmsOffset   offset      = extent->_lastRecordOffset ;
         extent->_recCount++ ;
         ++( _mbStatInfo[ context->mbID() ]._totalRecords ) ;
         if ( DMS_INVALID_OFFSET != offset )
         {
            dmsRecordRW preRW = record2RW( dmsRecordID( extRW.getExtentID(),
                                                        offset ),
                                           context->mbID() ) ;
            dmsRecord *preRecord = preRW.writePtr() ;
            preRecord->setNextOffset( myOffset ) ;
            pRecord->setPrevOffset( offset ) ;
         }
         extent->_lastRecordOffset = myOffset ;
         if ( DMS_INVALID_OFFSET == extent->_firstRecordOffset )
         {
            extent->_firstRecordOffset = myOffset ;
         }
      }

      _mbStatInfo[context->mbID()]._lastCompressRatio =
         (UINT8)( recordData.getCompressRatio() * 100 ) ;
      _mbStatInfo[context->mbID()]._totalOrgDataLen += recordData.orgLen() ;
      _mbStatInfo[context->mbID()]._totalDataLen += recordData.len() ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__EXTENTINSERTRECORD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__EXTENTREMOVERECORD, "_dmsStorageData::_extentRemoveRecord" )
   INT32 _dmsStorageData::_extentRemoveRecord( dmsMBContext *context,
                                               dmsExtRW &extRW,
                                               dmsRecordRW &recordRW,
                                               _pmdEDUCB *cb,
                                               BOOLEAN decCount )
   {
      INT32 rc              = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__EXTENTREMOVERECORD ) ;
      monAppCB * pMonAppCB  = cb ? cb->getMonAppCB() : NULL ;

      dmsExtent *pExtent = NULL ;
      const dmsRecord *pRecord = NULL ;
      dmsRecordID rid = recordRW.getRecordID() ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold exclusive lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      pExtent = extRW.writePtr<dmsExtent>() ;
      pRecord = recordRW.readPtr() ;

      if ( !pRecord->isOvt() )
      {
         dmsOffset prevRecordOffset = pRecord->getPrevOffset() ;
         dmsOffset nextRecordOffset = pRecord->getNextOffset() ;

         if ( DMS_INVALID_OFFSET != prevRecordOffset )
         {
            dmsRecordID prevRID = rid ;
            prevRID._offset = prevRecordOffset ;
            dmsRecordRW prevRW = record2RW( prevRID, context->mbID() ) ;
            dmsRecord *prevRecord = prevRW.writePtr() ;
            prevRecord->setNextOffset( nextRecordOffset ) ;
         }
         if ( DMS_INVALID_OFFSET != nextRecordOffset )
         {
            dmsRecordID nextRID = rid ;
            nextRID._offset = nextRecordOffset ;
            dmsRecordRW nextRW = record2RW( nextRID, context->mbID() ) ;
            dmsRecord *nextRecord = nextRW.writePtr() ;
            nextRecord->setPrevOffset( prevRecordOffset ) ;
         }
         if ( pExtent->_firstRecordOffset == rid._offset )
         {
            pExtent->_firstRecordOffset = nextRecordOffset ;
         }
         if ( pExtent->_lastRecordOffset == rid._offset )
         {
            pExtent->_lastRecordOffset = prevRecordOffset ;
         }

         if ( decCount )
         {
            --(pExtent->_recCount) ;
            --( _mbStatInfo[ context->mbID() ]._totalRecords ) ;
         }
      }
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;

      rc = _saveDeletedRecord( context->mb(), rid, 0 ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to save deleted record, rc = %d", rc ) ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__EXTENTREMOVERECORD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__FINALRECORDSIZE, "_dmsStorageData::_finalRecordSize" )
   void _dmsStorageData::_finalRecordSize( UINT32 &size,
                                           const dmsRecordData &recordData )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__FINALRECORDSIZE ) ;
      if ( _pStorageInfo && _pStorageInfo->_overflowRatio > 0 )
      {
         size += ( size * _pStorageInfo->_overflowRatio + 50 ) / 100 ;
      }
      else if ( !_pStorageInfo )
      {
         size = size * DMS_RECORD_OVERFLOW_RATIO ;
      }

      size += DMS_RECORD_METADATA_SZ ;
      size = OSS_MIN( DMS_RECORD_MAX_SZ, ossAlignX ( size, 4 ) ) ;
      PD_TRACE2 ( SDB__DMSSTORAGEDATA__FINALRECORDSIZE,
                  PD_PACK_STRING ( "size after align" ),
                  PD_PACK_UINT ( size ) ) ;
      PD_TRACE_EXIT( SDB__DMSSTORAGEDATA__FINALRECORDSIZE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__ONINSERTFAIL, "_dmsStorageData::_onInsertFail" )
   INT32 _dmsStorageData::_onInsertFail( dmsMBContext *context,
                                         BOOLEAN hasInsert,
                                         dmsRecordID rid, SDB_DPSCB *dpscb,
                                         ossValuePtr dataPtr, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__ONINSERTFAIL ) ;

      if ( hasInsert )
      {
         rc = deleteRecord( context, rid, dataPtr, cb, dpscb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to rollback, rc: %d", rc ) ;
      }
      else if ( rid.isValid() )
      {
         _saveDeletedRecord( context->mb(), rid, 0 ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA__ONINSERTFAIL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_EXTRACTDATA, "_dmsStorageData::extractData" )
   INT32 _dmsStorageData::extractData( dmsMBContext *mbContext,
                                       dmsRecordRW &recordRW,
                                       _pmdEDUCB *cb,
                                       dmsRecordData &recordData )
   {
      INT32 rc                = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA_EXTRACTDATA ) ;
      monAppCB * pMonAppCB    = cb ? cb->getMonAppCB() : NULL ;
      const dmsRecord *pRecord= recordRW.readPtr( 0 ) ;

      recordData.reset() ;

      if ( !mbContext->isMBLock() )
      {
         PD_LOG( PDERROR, "MB Context must be locked" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( pRecord->isOvf() )
      {
         dmsRecordID ovfRID = pRecord->getOvfRID() ;
         dmsRecordRW ovfRW = record2RW( ovfRID, mbContext->mbID() ) ;
         ovfRW.setNothrow( recordRW.isNothrow() ) ;
         pRecord = ovfRW.readPtr( 0 ) ;
         if ( NULL == pRecord )
         {
            rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
            PD_LOG( PDERROR, "Failed to get record from address[%d.%d]",
                    ovfRID._extent, ovfRID._offset ) ;
            goto error ;
         }
         SDB_ASSERT( pRecord->isOvt(), "Record must be ovt" ) ;
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_READ, 1 ) ;
      }

      recordData.setData( pRecord->getData(), pRecord->getDataLength(),
                          FALSE, TRUE ) ;

      if ( pRecord->isCompressed() )
      {
         const CHAR *pUncompressData = NULL ;
         INT32 unCompressDataLen = 0 ;
         rc = dmsUncompress( cb, &_compressorEntry[ mbContext->mbID() ],
                             pRecord->getData(), pRecord->getDataLength(),
                             &pUncompressData, &unCompressDataLen ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to uncompress data, rc: %d", rc ) ;
            goto error ;
         }
         if ( unCompressDataLen != *(INT32*)pUncompressData )
         {
            PD_LOG( PDERROR, "Uncompress data length[%d] is not unmatch "
                    "real length[%d]", unCompressDataLen,
                    *(INT32*)pUncompressData ) ;
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }
         recordData.setData( pUncompressData, unCompressDataLen,
                             FALSE, FALSE ) ;
      }
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_READ, 1 ) ;
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_READ, 1 ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA_EXTRACTDATA, rc ) ;
      return rc ;
   error:
      goto done ;

   }

   INT32 _dmsStorageData::_operationPermChk( DMS_ACCESS_TYPE accessType )
   {
      INT32 rc = SDB_OK ;
      if ( DMS_ACCESS_TYPE_POP == accessType )
      {
         PD_LOG( PDERROR, "Operation type[ %d ] is incompatible with "
                 "collection", accessType ) ;
         rc = SDB_OPERATION_INCOMPATIBLE ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _dmsStorageData::_getEyeCatcher() const
   {
      return DMS_DATASU_EYECATCHER ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_POSTEXTLOAD, "_dmsStorageData::postExtLoad" )
   void _dmsStorageData::postLoadExt( dmsMBContext *context,
                                      dmsExtent *extAddr,
                                      SINT32 extentID )
   {
      _mapExtent2DelList( context->mb(), extAddr, extentID ) ;
   }

   INT32 _dmsStorageData::dumpExtOptions( dmsMBContext *context,
                                          BSONObj &extOptions )
   {
      return SDB_OK ;
   }
}

