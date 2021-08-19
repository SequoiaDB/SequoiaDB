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

   Source File Name = dmsStorageLoadExtent.cpp

   Descriptive Name =

   When/how to use: load extent to database

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/10/2013  JW  Initial Draft

   Last Changed =

******************************************************************************/

#include "dms.hpp"
#include "dmsExtent.hpp"
#include "dmsSMEMgr.hpp"
#include "dmsStorageLoadExtent.hpp"
#include "dmsCompress.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "migLoad.hpp"
#include "dmsTransLockCallback.hpp"

using namespace bson ;

namespace engine
{
   void dmsStorageLoadOp::_initExtentHeader ( dmsExtent *extAddr,
                                              UINT16 numPages )
   {
      SDB_ASSERT ( _pageSize * numPages == _currentExtentSize,
                   "extent size doesn't match" ) ;
      extAddr->_eyeCatcher[0]          = DMS_EXTENT_EYECATCHER0 ;
      extAddr->_eyeCatcher[1]          = DMS_EXTENT_EYECATCHER1 ;
      extAddr->_blockSize              = numPages ;
      extAddr->_mbID                   = 0 ;
      extAddr->_flag                   = DMS_EXTENT_FLAG_INUSE ;
      extAddr->_version                = DMS_EXTENT_CURRENT_V ;
      extAddr->_logicID                = DMS_INVALID_EXTENT ;
      extAddr->_prevExtent             = DMS_INVALID_EXTENT ;
      extAddr->_nextExtent             = DMS_INVALID_EXTENT ;
      extAddr->_recCount               = 0 ;
      extAddr->_firstRecordOffset      = DMS_INVALID_EXTENT ;
      extAddr->_lastRecordOffset       = DMS_INVALID_EXTENT ;
      extAddr->_freeSpace              = _pageSize * numPages -
                                         sizeof(dmsExtent) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOADEXT__ALLOCEXTENT, "dmsStorageLoadOp::_allocateExtent" )
   INT32 dmsStorageLoadOp::_allocateExtent ( INT32 requestSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGELOADEXT__ALLOCEXTENT );
      if ( requestSize < DMS_MIN_EXTENT_SZ(_pageSize) )
      {
         requestSize = DMS_MIN_EXTENT_SZ(_pageSize) ;
      }
      else if ( requestSize > (INT32)DMS_MAX_EXTENT_SZ(_su->data()) )
      {
         requestSize = DMS_MAX_EXTENT_SZ(_su->data()) ;
      }
      else
      {
         requestSize = ossRoundUpToMultipleX ( requestSize, _pageSize ) ;
      }

      if ( (UINT32)requestSize > _buffSize && _pCurrentExtent )
      {
         SDB_OSS_FREE( _pCurrentExtent ) ;
         _pCurrentExtent = NULL ;
         _buffSize = 0 ;
      }

      if ( (UINT32)requestSize > _buffSize )
      {
         _pCurrentExtent = ( CHAR* )SDB_OSS_MALLOC( requestSize << 1 ) ;
         if ( !_pCurrentExtent )
         {
            PD_LOG( PDERROR, "Alloc memroy[%d] failed",
                    requestSize << 1 ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         _buffSize = ( requestSize << 1 ) ;
      }

      _currentExtentSize = requestSize ;
      _initExtentHeader ( (dmsExtent*)_pCurrentExtent,
                          _currentExtentSize/_pageSize ) ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGELOADEXT__ALLOCEXTENT, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOADEXT__IMPRTBLOCK, "dmsStorageLoadOp::pushToTempDataBlock" )
   INT32 dmsStorageLoadOp::pushToTempDataBlock ( dmsMBContext *mbContext,
                                                 pmdEDUCB *cb,
                                                 BSONObj &record,
                                                 BOOLEAN isLast,
                                                 BOOLEAN isAsynchr )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGELOADEXT__IMPRTBLOCK );
      UINT32      dmsrecordSize   = 0 ;
      dmsRecord   *pRecord        = NULL ;
      dmsRecord   *pPreRecord     = NULL ;
      dmsOffset   offset          = DMS_INVALID_OFFSET ;
      dmsOffset   recordOffset    = DMS_INVALID_OFFSET ;

      _IDToInsert oid ;
      idToInsertEle oidEle((CHAR*)(&oid)) ;
      CHAR *pNewRecordData       = NULL ;
      dmsRecordData recordData ;

      dmsCompressorEntry *compressorEntry = NULL ;

      SDB_ASSERT( mbContext, "mb context can't be NULL" ) ;

      compressorEntry = _su->data()->getCompressorEntry( mbContext->mbID() ) ;
      /* For concurrency protection with drop CL and set compresor. */
      dmsCompressorGuard compGuard( compressorEntry, SHARED ) ;

      try
      {
         recordData.setData( record.objdata(), record.objsize(),
                             UTIL_COMPRESSOR_INVALID, TRUE ) ;
         /* (0) */
         // verify whether the record got "_id" inside
         BSONElement ele = record.getField ( DMS_ID_KEY_NAME ) ;
         const CHAR *pCheckErr = "" ;
         if ( !dmsIsRecordIDValid( ele, TRUE, &pCheckErr ) )
         {
            PD_LOG( PDERROR, "Record[%s] _id is error: %s",
                    record.toString().c_str(), pCheckErr ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         // if the record is not for temp, and
         // "_id" doesn't exist, let's create the object
         if ( ele.eoo() )
         {
            oid._oid.init() ;
            rc = cb->allocBuff( oidEle.size() + record.objsize(),
                                &pNewRecordData ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Alloc memory[size:%u] failed, rc: %d",
                       oidEle.size() + record.objsize(), rc ) ;
               goto error ;
            }
            /// copy to new data
            *(UINT32*)pNewRecordData = oidEle.size() + record.objsize() ;
            ossMemcpy( pNewRecordData + sizeof(UINT32), oidEle.rawdata(),
                       oidEle.size() ) ;
            ossMemcpy( pNewRecordData + sizeof(UINT32) + oidEle.size(),
                       record.objdata() + sizeof(UINT32),
                       record.objsize() - sizeof(UINT32) ) ;
            recordData.setData( pNewRecordData,
                                oidEle.size() + record.objsize(),
                                UTIL_COMPRESSOR_INVALID, TRUE ) ;
            record = BSONObj( pNewRecordData ) ;
         }
         dmsrecordSize = recordData.len() ;

         // check
         if ( recordData.len() + DMS_RECORD_METADATA_SZ >
              DMS_RECORD_USER_MAX_SZ )
         {
            rc = SDB_DMS_RECORD_TOO_BIG ;
            goto error ;
         }

         if ( compressorEntry->ready() )
         {
            const CHAR *compressedData    = NULL ;
            INT32 compressedDataSize      = 0 ;
            UINT8 compressRatio           = 0 ;
            rc = dmsCompress( cb, compressorEntry,
                              recordData.data(), recordData.len(),
                              &compressedData, &compressedDataSize,
                              compressRatio ) ;
            // Compression is valid and ratio is less the threshold
            if ( SDB_OK == rc &&
                 compressedDataSize + sizeof(UINT32) < recordData.orgLen() &&
                 compressRatio < UTIL_COMPRESSOR_DFT_MIN_RATIO )
            {
               // 4 bytes len + compressed record
               dmsrecordSize = compressedDataSize + sizeof(UINT32) ;
               // set the compression data
               recordData.setData( compressedData, compressedDataSize,
                                   compressorEntry->getCompressorType(),
                                   FALSE ) ;
            }
            else if ( rc )
            {
               // In any case of error, leave it, and use the original data.
               if ( SDB_UTIL_COMPRESS_ABORT == rc )
               {
                  PD_LOG( PDINFO, "Record compression aborted. "
                          "Insert the original data. rc: %d", rc ) ;
               }
               else
               {
                  PD_LOG( PDWARNING, "Record compression failed. "
                          "Insert the original data. rc: %d", rc ) ;
               }
               rc = SDB_OK ;
            }
         }

         /*
          * Release the guard to avoid deadlock with truncate/drop collection.
          */
         compGuard.release() ;

         // add record metadata and oid
         dmsrecordSize *= DMS_RECORD_OVERFLOW_RATIO ;
         dmsrecordSize += DMS_RECORD_METADATA_SZ ;
         // record is ALWAYS 4 bytes aligned
         dmsrecordSize = OSS_MIN( DMS_RECORD_MAX_SZ,
                                  ossAlignX ( dmsrecordSize, 4 ) ) ;

         INT32 expandSize = dmsrecordSize << DMS_RECORDS_PER_EXTENT_SQUARE ;
         if ( expandSize > DMS_BEST_UP_EXTENT_SZ )
         {
            expandSize = expandSize < DMS_BEST_UP_EXTENT_SZ ?
                         DMS_BEST_UP_EXTENT_SZ : expandSize ;
         }

         if ( !_pCurrentExtent )
         {
            rc = _allocateExtent ( expandSize ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to allocate new extent in reorg file, "
                        "rc = %d", rc ) ;
               goto error ;
            }
            _currentExtent = (dmsExtent*)_pCurrentExtent ;
         }

         if ( dmsrecordSize > (UINT32)_currentExtent->_freeSpace || isLast )
         {
            // lock
            rc = mbContext->mbLock( EXCLUSIVE ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to lock collection, rc=%d", rc ) ;
               goto error ;
            }
            if ( !isAsynchr )
            {
               _currentExtent->_firstRecordOffset = DMS_INVALID_OFFSET ;
               _currentExtent->_lastRecordOffset = DMS_INVALID_OFFSET ;
            }

            rc = _su->loadExtentA( mbContext, _pCurrentExtent,
                                   _currentExtentSize / _pageSize,
                                   TRUE ) ;
            // unlock
            mbContext->mbUnlock() ;

            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to load extent, rc = %d", rc ) ;
               goto error ;
            }

            if ( isLast )
            {
               goto done ;
            }

            rc = _allocateExtent ( expandSize ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to allocate new extent in reorg file, "
                        "rc = %d", rc ) ;
               goto error ;
            }
         }

         recordOffset = _currentExtentSize - _currentExtent->_freeSpace ;
         pRecord = ( dmsRecord* )( (const CHAR*)_currentExtent + recordOffset ) ;

         if ( _currentExtent->_freeSpace - (INT32)dmsrecordSize <
              (INT32)DMS_MIN_RECORD_SZ &&
              _currentExtent->_freeSpace <= (INT32)DMS_RECORD_MAX_SZ )
         {
            dmsrecordSize = _currentExtent->_freeSpace ;
         }

         // set record header
         pRecord->setNormal() ;
         pRecord->setMyOffset( recordOffset ) ;
         pRecord->setSize( dmsrecordSize ) ;
         pRecord->setData( recordData ) ;
         pRecord->setNextOffset( DMS_INVALID_OFFSET ) ;
         pRecord->setPrevOffset( DMS_INVALID_OFFSET ) ;

         // set extent header
         if ( isAsynchr )
         {
            _currentExtent->_recCount++ ;
         }
         _currentExtent->_freeSpace -= dmsrecordSize ;
         // set previous record next pointer
         offset = _currentExtent->_lastRecordOffset ;
         if ( DMS_INVALID_OFFSET != offset )
         {
            pPreRecord = (dmsRecord*)( (const CHAR*)_currentExtent + offset ) ;
            pPreRecord->setNextOffset( recordOffset ) ;
            pRecord->setPrevOffset( offset ) ;
         }
         _currentExtent->_lastRecordOffset = recordOffset ;

         // then check extent header for first record
         offset = _currentExtent->_firstRecordOffset ;
         if ( DMS_INVALID_OFFSET == offset )
         {
            _currentExtent->_firstRecordOffset = recordOffset ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGELOADEXT__IMPRTBLOCK, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOADEXT__LDDATA, "dmsStorageLoadOp::loadBuildPhase" )
   INT32 dmsStorageLoadOp::loadBuildPhase ( dmsMBContext *mbContext,
                                            pmdEDUCB *cb,
                                            BOOLEAN isAsynchr,
                                            migMaster *pMaster,
                                            UINT32 *success,
                                            UINT32 *failure )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGELOADEXT__LDDATA ) ;

      dmsExtRW       extRW ;
      dmsRecordRW    recordRW ;
      dmsExtent      *extAddr       = NULL ;
      dmsRecordID    recordID ;
      dmsRecord      *pRecord       = NULL ;
      dmsRecordData  recordData ;
      dmsOffset      recordOffset   = DMS_INVALID_OFFSET ;
      dmsExtentID    tempExtentID   = 0 ;
      monAppCB       *pMonAppCB     = cb ? cb->getMonAppCB() : NULL ;

      dmsTransLockCallback callback( pmdGetKRCB()->getTransCB(),
                                     cb ) ;

      SDB_ASSERT ( _su, "_su can't be NULL" ) ;
      SDB_ASSERT ( mbContext, "dms mb context can't be NULL" ) ;
      SDB_ASSERT ( cb, "cb is NULL" ) ;

      rc = mbContext->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection, rc: %d", rc ) ;

      if ( DMS_INVALID_EXTENT == mbContext->mb()->_loadFirstExtentID &&
           DMS_INVALID_EXTENT == mbContext->mb()->_loadLastExtentID )
      {
         PD_LOG ( PDEVENT, "has not load extent" ) ;
         goto done ;
      }

      if ( ( DMS_INVALID_EXTENT == mbContext->mb()->_lastExtentID &&
             DMS_INVALID_EXTENT != mbContext->mb()->_firstExtentID ) ||
           ( DMS_INVALID_EXTENT != mbContext->mb()->_lastExtentID &&
             DMS_INVALID_EXTENT == mbContext->mb()->_firstExtentID ) )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "Invalid mb context[%s], first extent: %d, last "
                  "extent: %d", mbContext->toString().c_str(),
                  mbContext->mb()->_firstExtentID,
                  mbContext->mb()->_lastExtentID ) ;
         goto error ;
      }

      callback.setIDInfo( _su->data()->CSID(), mbContext->mbID(),
                          _su->data()->logicalID(),
                          mbContext->clLID() ) ;

      clearFlagLoadLoad ( mbContext->mb() ) ;
      setFlagLoadBuild ( mbContext->mb() ) ;

      while ( !cb->isForced() )
      {
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "dms mb context lock failed, rc", rc ) ;
            goto error ;
         }

         tempExtentID = mbContext->mb()->_loadFirstExtentID ;
         if ( DMS_INVALID_EXTENT == tempExtentID )
         {
            mbContext->mb()->_loadFirstExtentID = DMS_INVALID_EXTENT ;
            mbContext->mb()->_loadLastExtentID = DMS_INVALID_EXTENT ;
            goto done ;
         }

         extRW = _su->data()->extent2RW( tempExtentID, mbContext->mbID() ) ;
         extRW.setNothrow( TRUE ) ;
         extAddr = extRW.writePtr<dmsExtent>() ;
         PD_CHECK ( extAddr, SDB_SYS, error, PDERROR, "Invalid extent: %d",
                    tempExtentID ) ;
         SDB_ASSERT( extAddr->validate( mbContext->mbID() ),
                     "Invalid extent" ) ;

         rc = _su->data()->addExtent2Meta( tempExtentID, extAddr, mbContext ) ;
         PD_RC_CHECK( rc, PDERROR, "Add extent to meta failed, rc: %d", rc ) ;

         mbContext->mb()->_loadFirstExtentID = extAddr->_nextExtent ;

         extAddr->_firstRecordOffset = DMS_INVALID_OFFSET ;
         extAddr->_lastRecordOffset  = DMS_INVALID_OFFSET ;
         _su->addExtentRecordCount( mbContext->mb(), extAddr->_recCount ) ;
         extAddr->_recCount          = 0 ;
         _su->data()->postLoadExt( mbContext, extAddr, tempExtentID ) ;

         recordOffset = DMS_EXTENT_METADATA_SZ ;
         recordID._extent = tempExtentID ;

         while ( DMS_INVALID_OFFSET != recordOffset )
         {
            recordID._offset = recordOffset ;
            recordRW = _su->data()->record2RW( recordID, mbContext->mbID() ) ;
            recordRW.setNothrow( TRUE ) ;
            pRecord = recordRW.writePtr( 0 ) ;
            if ( !pRecord )
            {
               PD_LOG( PDERROR, "Get record write address failed" ) ;
               rc = SDB_SYS ;
               goto rollback ;
            }

            rc = _su->data()->extractData( mbContext, recordRW,
                                           cb, recordData ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Extract record data failed, rc: %d", rc ) ;
               goto rollback ;
            }

            recordOffset = pRecord->getNextOffset() ;
            ++( extAddr->_recCount ) ;

            try
            {
               // get the BSON object
               BSONObj obj ( recordData.data() ) ;
               // when we get here, that means we have a new record
               // to add to index
               DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;

               // attempt to insert into the index
               rc = _su->index()->indexesInsert( mbContext, tempExtentID, obj,
                                                 recordID, cb, &callback ) ;
               // if any error happen
               if ( rc )
               {
                  if ( SDB_IXM_DUP_KEY != rc )
                  {
                     PD_LOG ( PDERROR, "Failed to insert into index, rc=%d",
                              rc ) ;
                     goto rollback ;
                  }
                  if ( success )
                  {
                     --(*success) ;
                  }
                  if ( failure )
                  {
                     ++(*failure) ;
                  }
                  if ( pMaster )
                  {
                     rc = pMaster->sendMsgToClient( "Error: index insert, error"
                                                    "code %d, %s", rc,
                                                    obj.toString().c_str() ) ;
                     if ( rc )
                     {
                        PD_LOG ( PDERROR, "Failed to send msg, rc=%d", rc ) ;
                     }
                  }
                  rc = _su->data()->deleteRecord ( mbContext, recordID,
                                                   (ossValuePtr)recordData.data(),
                                                   cb, NULL, NULL ) ;
                  if ( rc )
                  {
                     PD_LOG ( PDERROR, "Failed to rollback, rc = %d", rc ) ;
                     goto rollback ;
                  }
               }
            }
            catch ( std::exception &e )
            {
               PD_LOG ( PDERROR, "Failed to create BSON object: %s",
                        e.what() ) ;
               rc = SDB_SYS ;
               goto rollback ;
            }

            // extent point to cur record
            if ( DMS_INVALID_OFFSET == extAddr->_firstRecordOffset )
            {
               extAddr->_firstRecordOffset = recordID._offset ;
            }
            extAddr->_lastRecordOffset = recordID._offset ;
         } //while ( DMS_INVALID_OFFSET != recordOffset )

         // unlock
         mbContext->mbUnlock() ;
      } // while

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGELOADEXT__LDDATA, rc );
      return rc ;
   error:
      goto done ;
   rollback:
      // save the extent other record to del list
      recordOffset = recordID._offset ;
      const dmsRecord *pReadRecord = NULL ;
      while ( DMS_INVALID_OFFSET != recordOffset )
      {
         recordID._offset = recordOffset ;
         recordRW = _su->data()->record2RW( recordID, mbContext->mbID() ) ;
         pReadRecord = recordRW.readPtr() ;
         recordOffset = pReadRecord->getNextOffset() ;

         _su->extentRemoveRecord( mbContext, extRW, recordRW, cb ) ;

         if ( DMS_INVALID_OFFSET != recordOffset )
         {
            ++( extAddr->_recCount ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOADEXT__ROLLEXTENT, "dmsStorageLoadOp::loadRollbackPhase" )
   INT32 dmsStorageLoadOp::loadRollbackPhase( dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGELOADEXT__ROLLEXTENT ) ;

      SDB_ASSERT ( _su, "_su can't be NULL" ) ;
      SDB_ASSERT ( mbContext, "mb context can't be NULL" ) ;

      PD_LOG ( PDEVENT, "Start loadRollbackPhase" ) ;

      rc = _su->data()->truncateCollectionLoads( NULL, mbContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate collection loads, rc: %d",
                   rc ) ;

   done:
      PD_LOG ( PDEVENT, "End loadRollbackPhase" ) ;
      PD_TRACE_EXITRC ( SDB__DMSSTORAGELOADEXT__ROLLEXTENT, rc );
      return rc ;
   error:
      goto done ;
   }

}

