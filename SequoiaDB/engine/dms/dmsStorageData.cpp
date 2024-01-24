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
#include "pdSecure.hpp"

namespace engine
{

   _dmsStorageData::_dmsStorageData( IStorageService *service,
                                     dmsSUDescriptor *suDescriptor,
                                     const CHAR *pSuFileName,
                                     _IDmsEventHolder *pEventHolder )
   : _dmsStorageDataCommon( service, suDescriptor, pSuFileName, pEventHolder )
   {
   }

   _dmsStorageData::~_dmsStorageData()
   {
   }

   INT32 _dmsStorageData::_prepareAddCollection( const BSONObj *extOption,
                                                 dmsCreateCLOptions &options )
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__CHKINSERTDATA, "_dmsStorageData::_checkInsertData" )
   INT32 _dmsStorageData::_checkInsertData( const BSONObj &record,
                                            BOOLEAN mustOID,
                                            pmdEDUCB *cb,
                                            dmsRecordData &recordData,
                                            BOOLEAN &memReallocate,
                                            INT64 position )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__CHKINSERTDATA ) ;

      IDToInsert oid ;
      idToInsertEle oidEle((CHAR*)(&oid)) ;
      CHAR *pMergedData = NULL ;

      memReallocate = FALSE ;

      try
      {
          // Step 1: Prepare the data, add OID and compress if necessary.
         recordData.setData( record.objdata(), record.objsize() ) ;
         BSONElement ele = record.getField( DMS_ID_KEY_NAME ) ;
         // check ID index for normal update
         // NOTE: for sequoiadb upgrade, if the old data before upgrade
         //       contains invalid _id field, we could not report error,
         //       we need to allow update if _id field is not changed
         if( !cb->isDoReplay() &&
             !cb->isInTransRollback() &&
             !cb->isDoRollback() )
         {
            const CHAR *pCheckErr = "" ;
            if ( !dmsIsRecordIDValid( ele, TRUE, &pCheckErr ) )
            {
               PD_LOG_MSG( PDERROR, "_id is error: %s", pCheckErr ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         // judge must oid
         if ( mustOID && ele.eoo() )
         {
            IDToInsert oid ;
            idToInsertEle oidEle((CHAR*)(&oid)) ;

            oid._oid.init() ;
            rc = cb->allocBuff( oidEle.size() + record.objsize(), &pMergedData ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Alloc memory[size:%u] failed, rc: %d",
                       oidEle.size() + record.objsize(), rc ) ;
               goto error ;
            }
            /// copy to new data
            *(UINT32*)pMergedData = oidEle.size() + record.objsize() ;
            ossMemcpy( pMergedData + sizeof(UINT32), oidEle.rawdata(),
                       oidEle.size() ) ;
            ossMemcpy( pMergedData + sizeof(UINT32) + oidEle.size(),
                       record.objdata() + sizeof(UINT32),
                       record.objsize() - sizeof(UINT32) ) ;
            recordData.setData( pMergedData,
                                oidEle.size() + record.objsize() ) ;
            memReallocate = TRUE ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to prepare insert data, occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA__CHKINSERTDATA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__PREPAREINSERT, "_dmsStorageData::_prepareInsert" )
   INT32 _dmsStorageData::_prepareInsert( const dmsRecordID &recordID,
                                          const dmsRecordData &recordData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__PREPAREINSERT ) ;

      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA__PREPAREINSERT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__GETRECPOS, "_dmsStorageData::_getRecordPosition" )
   INT32 _dmsStorageData::_getRecordPosition( dmsMBContext *context,
                                              const dmsRecordID &rid,
                                              const dmsRecordData &recordData,
                                              pmdEDUCB *cb,
                                              INT64 &position )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__GETRECPOS ) ;

      if ( rid.isValid() )
      {
         position = (INT64)( ossPack32To64( (UINT32)( rid._extent ),
                                            (UINT32)( rid._offset ) ) ) ;
      }
      else
      {
         position = -1 ;
      }

      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA__GETRECPOS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__CHKREUSEPOS, "_dmsStorageData::_checkReusePosition" )
   INT32 _dmsStorageData::_checkReusePosition( dmsMBContext *context,
                                               const DPS_TRANS_ID &transID,
                                               pmdEDUCB *cb,
                                               INT64 &position,
                                               dmsRecordID &foundRID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__CHKREUSEPOS ) ;

      BOOLEAN isFound = FALSE ;

      /// when is rollback, and the rid is found
      if ( -1 != position &&
           DPS_INVALID_TRANS_ID != transID &&
           cb->isInTransRollback() &&
           !cb->isTakeOverTransRB() )
      {
         dmsRecordData recordData ;
         dmsRecordID tmpRID ;

         // use the given position instead
         ossUnpack32From64( (UINT64)position,
                            (UINT32 &)( tmpRID._extent ),
                            (UINT32 &)( tmpRID._offset ) ) ;

         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock collection, rc: %d", rc ) ;

         if ( SDB_DMS_RECORD_NOTEXIST ==
                     _dmsStorageDataCommon::extractData( context, tmpRID, cb,
                                                         recordData, FALSE, FALSE ) )
         {
            foundRID = tmpRID ;
            isFound = TRUE ;
         }

         context->mbUnlock() ;
      }

      // can not reuse position, the position should be cleared
      if ( !isFound )
      {
         position = -1 ;
         foundRID.reset() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA__CHKREUSEPOS, rc ) ;
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

      UINT64 tmpRID = context->mbStat()->_ridGen.inc() ;
      foundRID.fromUINT64( tmpRID ) ;

      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA__ALLOCRECORDSPACE, rc ) ;

      return rc ;
   }

   INT32 _dmsStorageData::_checkRecordSpace( dmsMBContext *context,
                                             UINT32 size,
                                             dmsRecordID &foundRID,
                                             pmdEDUCB *cb )
   {
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__RESERVEFROMDELETELIST, "_dmsStorageData::_reserveFromDeleteList" )
   INT32 _dmsStorageData::_reserveFromDeleteList( dmsMBContext *context,
                                                  UINT32 requiredSize,
                                                  dmsRecordID &resultID,
                                                  pmdEDUCB * cb )
   {
      INT32 rc                      = SDB_OK ;
      UINT8  deleteRecordSlot       = 0 ;
      const static INT32 s_maxSearch = 3 ;

      INT32  j                      = 0 ;
      INT32  i                      = 0 ;
      dpsTransCB *pTransCB          = pmdGetKRCB()->getTransCB() ;
      dpsTransRetInfo retInfo ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__RESERVEFROMDELETELIST ) ;
      PD_TRACE1 ( SDB__DMSSTORAGEDATA__RESERVEFROMDELETELIST,
                  PD_PACK_UINT ( requiredSize ) ) ;
      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      deleteRecordSlot = dmsMBGetSpaceSlot( requiredSize ) ;
      SDB_ASSERT( deleteRecordSlot < dmsMB::_max, "Invalid record size" ) ;

      if ( deleteRecordSlot >= dmsMB::_max )
      {
         PD_LOG( PDERROR, "Invalid record size: %u", requiredSize ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   retry:
      rc = SDB_DMS_NOSPC ;
      try
      {
         for ( j = deleteRecordSlot ; j < dmsMB::_max ; ++j )
         {
            dmsRecordID foundDeletedID  ;
            dmsRecordRW preRW ;
            BOOLEAN startFromHead = FALSE ;

            // if searching the slot of the last search position,
            // we can try to start from last search position
            if ( j == context->mbStat()->_lastSearchSlot )
            {
               if ( j == deleteRecordSlot )
               {
                  // it is the first loop, we can use last search position
                  dmsRecordID lastRID = context->mbStat()->_lastSearchRID ;
                  if ( lastRID.isValid() )
                  {
                     dmsRecordRW lastRW = record2RW( lastRID, context->mbID() ) ;
                     const dmsDeletedRecord *pLast =
                                             lastRW.readPtr<dmsDeletedRecord>() ;
                     SDB_ASSERT( pLast->isDeleted(),
                                 "last search position should be deleted" ) ;
                     if ( pLast->isDeleted() )
                     {
                        foundDeletedID = pLast->getNextRID() ;
                        if ( foundDeletedID.isValid() )
                        {
                           preRW = lastRW ;
                        }
                     }
                     else
                     {
                        // the last search position is not deleted any more,
                        // just clear last search info
                        context->mbStat()->_lastSearchSlot = dmsMB::_max ;
                        context->mbStat()->_lastSearchRID.reset() ;
                     }
                  }
               }
               else
               {
                  // not the first loop, if we start from the search position,
                  // the header record of this slot may never be touched, so we
                  // clear the last search position, and start from the header
                  context->mbStat()->_lastSearchSlot = dmsMB::_max ;
                  context->mbStat()->_lastSearchRID.reset() ;
               }
            }

            if ( foundDeletedID.isNull() )
            {
               // get the first delete record from delete list
               foundDeletedID = context->mb()->_deleteList[j] ;
               // mark start from head
               startFromHead = TRUE ;
            }

            for ( i = 0 ; i < s_maxSearch ; ++i )
            {
               dmsRecordRW delRecordRW ;
               const dmsDeletedRecord* pRead = NULL ;

               if ( foundDeletedID.isNull() )
               {
                  // if we don't get a valid record id
                  if ( startFromHead )
                  {
                     // we already started from head, break to get next slot
                     break ;
                  }
                  else
                  {
                     // we started from middle, try restart
                     foundDeletedID = context->mb()->_deleteList[j] ;
                     preRW = dmsRecordRW() ;
                     // mark start from head ( we only restart once )
                     startFromHead = TRUE ;
                  }
               }

               delRecordRW = record2RW( foundDeletedID, context->mbID() ) ;
               pRead = delRecordRW.readPtr<dmsDeletedRecord>() ;

               // once the extent is valid, let's check the record is deleted
               // and got sufficient size for us
               if( pRead->isDeleted() && pRead->getSize() >= requiredSize )
               {
                  if ( !isTransLockRequired( context ) ||
                       SDB_OK == pTransCB->transLockTestX( cb, _logicalCSID,
                                                           context->mbID(),
                                                           &foundDeletedID,
                                                           &retInfo,
                                                           NULL,
                                                           FALSE ) )
                  {
                     if ( preRW.isEmpty() )
                     {
                        // it's just the first one from delete list, let's get it
                        context->mb()->_deleteList[j] = pRead->getNextRID() ;
                        // save the last search position
                        if ( j == deleteRecordSlot )
                        {
                           context->mbStat()->_lastSearchSlot = deleteRecordSlot ;
                           context->mbStat()->_lastSearchRID.reset() ;
                        }
                     }
                     else
                     {
                        dmsDeletedRecord *preWrite =
                           preRW.writePtr<dmsDeletedRecord>() ;
                        // we need to link the previous delete record to the next
                        preWrite->setNextRID( pRead->getNextRID() ) ;
                        // save the last search position
                        if ( j == deleteRecordSlot )
                        {
                           context->mbStat()->_lastSearchSlot = deleteRecordSlot ;
                           context->mbStat()->_lastSearchRID = preRW.getRecordID() ;
                        }
                     }

                     // change extent free space
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
                     // can't increase i counter
                     --i ;
                  }
               }

               //for some reason this slot can't be reused, let's get to the next
               preRW = delRecordRW ;
               foundDeletedID = pRead->getNextRID() ;
            }

            // save the last search position
            if ( j == deleteRecordSlot )
            {
               if ( preRW.isEmpty() )
               {
                  // has no previous record, we start the next search from
                  // the header of delete list anywat
                  // so no need to save the last search position
                  context->mbStat()->_lastSearchSlot = dmsMB::_max ;
                  context->mbStat()->_lastSearchRID.reset() ;
               }
               else
               {
                  // has previous record, we can start the next search from
                  // this previous record
                  // so save the last search position with previous record
                  context->mbStat()->_lastSearchSlot = deleteRecordSlot ;
                  context->mbStat()->_lastSearchRID = preRW.getRecordID() ;
               }
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

      // no space, need to allocate extent
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
                                              dmsDeletedRecord *pRecord,
                                              BOOLEAN isRecycle )
   {
      UINT8 deleteRecordSlot = 0 ;
      BOOLEAN isSaved = FALSE ;
      UINT16 mbID = mb->_blockID ;
      dmsMBStatInfo &mbStatInfo = _mbStatInfo[ mbID ] ;

      SDB_ASSERT( extAddr && pRecord, "NULL Pointer" ) ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD ) ;
      PD_TRACE6 ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD,
                  PD_PACK_STRING ( "offset" ),
                  PD_PACK_INT ( rid._offset ),
                  PD_PACK_STRING ( "recordSize" ),
                  PD_PACK_INT ( recordSize ),
                  PD_PACK_STRING ( "extentID" ),
                  PD_PACK_INT ( rid._extent ) ) ;

      // assign flags to the memory
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

      // change free space
      extAddr->_freeSpace += recordSize ;
      mbStatInfo._totalDataFreeSpace += recordSize ;

      // let's count which delete slots it fits
      deleteRecordSlot = dmsMBGetSpaceSlot( recordSize ) ;
      // make sure we don't mis calculated it
      SDB_ASSERT ( deleteRecordSlot < dmsMB::_max, "Invalid record size" ) ;

      if ( isRecycle &&
           mbStatInfo._lastSearchSlot == deleteRecordSlot &&
           mbStatInfo._lastSearchRID.isValid() )
      {
         // insert the current record to the last search position,
         // so the next search can check from this position
         dmsRecordRW lastRecordRW = record2RW( mbStatInfo._lastSearchRID,
                                               mbID ) ;
         dmsDeletedRecord* lastRecord =
                                 lastRecordRW.writePtr<dmsDeletedRecord>() ;
         SDB_ASSERT( lastRecord->isDeleted(),
                     "last search position should be deleted" ) ;
         if ( lastRecord->isDeleted() )
         {
            pRecord->setNextRID( lastRecord->getNextRID() ) ;
            lastRecord->setNextRID( rid ) ;
            isSaved = TRUE ;
         }
      }

      if ( !isSaved )
      {
         // set the first matching delete slot to the
         // next rid for the deleted record
         pRecord->setNextRID( mb->_deleteList [ deleteRecordSlot ] ) ;
         // Then assign MB delete slot to the extent and offset
         mb->_deleteList[ deleteRecordSlot ] = rid ;
      }

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
                                   pExtent, pRecord, TRUE ) ;
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

      // calculate the delete record size we need to use
      extentSize          = extAddr->_blockSize << pageSizeSquareRoot() ;
      extentUseableSpace  = extAddr->_freeSpace ;
      extAddr->_freeSpace = 0 ;

      // make sure the delete record is not greater 16MB
      deleteRecordSize    = OSS_MIN ( extentUseableSpace,
                                      DMS_RECORD_MAX_SZ ) ;
      // place first record offset
      recordOffset        = extentSize - extentUseableSpace ;
      curUseableSpace     = extentUseableSpace ;

      /// extentUseableSpace > 16MB
      while ( curUseableSpace - deleteRecordSize >=
              (INT32)DMS_MIN_DELETEDRECORD_SZ )
      {
         dmsRecordID rid( extentID, recordOffset ) ;
         dmsRecordRW rRW = record2RW( rid, mb->_blockID ) ;
         _saveDeletedRecord( mb, rid, deleteRecordSize,
                             extAddr, rRW.writePtr<dmsDeletedRecord>(),
                             FALSE ) ;
         curUseableSpace -= deleteRecordSize ;
         recordOffset += deleteRecordSize ;
      }
      /// 16MB < curUseableSpace < 16MB + DMS_MIN_DELETEDRECORD_SZ
      if ( curUseableSpace > deleteRecordSize )
      {
         dmsRecordID rid( extentID, recordOffset ) ;
         dmsRecordRW rRW = record2RW( rid, mb->_blockID ) ;
         _saveDeletedRecord( mb, rid, DMS_PAGE_SIZE4K,
                             extAddr, rRW.writePtr<dmsDeletedRecord>(),
                             FALSE ) ;
         curUseableSpace -= DMS_PAGE_SIZE4K ;
         recordOffset += DMS_PAGE_SIZE4K ;
      }
      /// 0 < curUseableSpace < 16MB
      if ( curUseableSpace > 0 )
      {
         dmsRecordID rid( extentID, recordOffset ) ;
         dmsRecordRW rRW = record2RW( rid, mb->_blockID ) ;
         _saveDeletedRecord( mb, rid, curUseableSpace,
                             extAddr, rRW.writePtr<dmsDeletedRecord>(),
                             FALSE ) ;
      }

      // correct check
      SDB_ASSERT( extentUseableSpace == extAddr->_freeSpace,
                  "Extent[%d] free space invalid" ) ;
   done :
      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATA__MAPEXTENT2DELLIST ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__FINALRECORDSIZE, "_dmsStorageData::_finalRecordSize" )
   void _dmsStorageData::_finalRecordSize( UINT32 &size,
                                           const dmsRecordData &recordData )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__FINALRECORDSIZE ) ;

      _overflowSize( size ) ;

      size += DMS_RECORD_METADATA_SZ ;
      // record is ALWAYS 4 bytes aligned
      size = OSS_MIN( DMS_RECORD_MAX_SZ, ossAlignX ( size, 4 ) ) ;
      PD_TRACE2 ( SDB__DMSSTORAGEDATA__FINALRECORDSIZE,
                  PD_PACK_STRING ( "size after align" ),
                  PD_PACK_UINT ( size ) ) ;
      PD_TRACE_EXIT( SDB__DMSSTORAGEDATA__FINALRECORDSIZE ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_POSTEXTLOAD, "_dmsStorageData::postLoadExt" )
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

   INT32 _dmsStorageData::setExtOptions ( dmsMBContext * context,
                                          const BSONObj & extOptions )
   {
      return SDB_OK ;
   }

}
