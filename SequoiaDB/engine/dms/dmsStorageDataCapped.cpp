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
   _dmsStorageDataCapped::_dmsStorageDataCapped( IStorageService *service,
                                                 dmsSUDescriptor *suDescriptor,
                                                 const CHAR* pSuFileName,
                                                 _IDmsEventHolder *pEventHolder )
   : _dmsStorageDataCommon( service, suDescriptor, pSuFileName, pEventHolder )
   {
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__PREPAREADDCOLLECTION, "_dmsStorageDataCapped::_prepareAddCollection" )
   INT32 _dmsStorageDataCapped::_prepareAddCollection( const BSONObj *extOption,
                                                       dmsCreateCLOptions &options )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__PREPAREADDCOLLECTION ) ;

      rc = _parseExtendOptions( extOption, options._cappedOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse options, rc: %d", rc ) ;

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

      dmsCappedCLOptions options ;

      rc = _parseExtendOptions( extOption, options ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse options, rc: %d", rc ) ;

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__CHKINSERTDATA, "_dmsStorageDataCapped::_checkInsertData" )
   INT32 _dmsStorageDataCapped::_checkInsertData( const BSONObj &record,
                                                  BOOLEAN mustOID,
                                                  pmdEDUCB *cb,
                                                  dmsRecordData &recordData,
                                                  BOOLEAN &memReallocate,
                                                  INT64 position )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__CHKINSERTDATA ) ;
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
         recordData.setData( mergedData, totalSize ) ;
         memReallocate = TRUE ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__CHKINSERTDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__PREPAREINSERT, "_dmsStorageDataCapped::_prepareInsert" )
   INT32 _dmsStorageDataCapped::_prepareInsert( const dmsRecordID &recordID,
                                                const dmsRecordData &recordData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__PREPAREINSERT ) ;

      // Force set the logical id in the record. Logical id is always at the
      // beginning of the record.
      LogicalIDToInsertEle ele( (CHAR *)( recordData.data() + sizeof(UINT32) ) ) ;
      UINT64 *lidPtr = (UINT64 *)ele.value() ;
      *lidPtr = recordID.toUINT64() ;

      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__PREPAREINSERT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__GETRECPOS, "_dmsStorageDataCapped::_getRecordPosition" )
   INT32 _dmsStorageDataCapped::_getRecordPosition( dmsMBContext *context,
                                                    const dmsRecordID &rid,
                                                    const dmsRecordData &recordData,
                                                    pmdEDUCB *cb,
                                                    INT64 &position )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__GETRECPOS ) ;

      dmsRecordID lastRID ;

      rc = context->getCollPtr()->getMaxRecordID( lastRID, cb ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get max record id, rc: %d", rc ) ;

      if ( lastRID.isValid() && rid == lastRID )
      {
         position = rid.toUINT64() ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Only allowed to delete the last record in "
                 "capped collection, rc: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__GETRECPOS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__CHKREUSEPOS, "_dmsStorageDataCapped::_checkReusePosition" )
   INT32 _dmsStorageDataCapped::_checkReusePosition( dmsMBContext *context,
                                                     const DPS_TRANS_ID &transID,
                                                     pmdEDUCB *cb,
                                                     INT64 &position,
                                                     dmsRecordID &foundRID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__CHKREUSEPOS ) ;

      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__CHKREUSEPOS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__FINALRECORDSIZE, "_dmsStorageDataCapped::_finalRecordSize" )
   void _dmsStorageDataCapped::_finalRecordSize( UINT32 &size,
                                                 const dmsRecordData &recordData )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__FINALRECORDSIZE ) ;
      // Append the logic id size. Only do that when the data is not compressed.
      // In case of data compressed, logical id is only stored in the record
      // header, and will be copied out when fetch the data.
      size += sizeof( LogicalIDToInsert ) ;
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

      UINT64 tmpID = 0 ;

      SDB_ASSERT( context, "Context should not be NULL" ) ;
      SDB_ASSERT( cb, "edu cb should not be NULL" ) ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold exclusive lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      rc = _limitProcess( context, size, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process limit, rc: %d", rc ) ;

      tmpID = context->mbStat()->_ridGen.add( size ) ;
      foundRID.fromUINT64( tmpID ) ;

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
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__CHKRECSPACE, "_dmsStorageDataCapped::_checkRecordSpace" )
   INT32 _dmsStorageDataCapped::_checkRecordSpace( dmsMBContext *context,
                                                   UINT32 size,
                                                   dmsRecordID &foundRID,
                                                   pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__CHKRECSPACE ) ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold exclusive lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      rc = _limitProcess( context, size, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process limit, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__CHKRECSPACE, rc ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__LIMITPROCESS, "_dmsStorageDataCapped::_limitProcess" )
   INT32 _dmsStorageDataCapped::_limitProcess( dmsMBContext *context,
                                               UINT32 sizeReq,
                                               pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__LIMITPROCESS ) ;

      // Check if exceeding the Size and Max limitations.
      // If yes, return error or recycle extent, depending on the option.
      if ( _numExceedLimit( context, 1 ) )
      {
         if ( _overwriteOnExceed( context ) )
         {
            // rc = _recycleOneExtent( context ) ;
            // PD_RC_CHECK( rc, PDERROR,
            //              "Recycle the eldest extent failed[ %d ]", rc ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Record number exceed the limit" ) ;
            rc = SDB_OSS_UP_TO_LIMIT ;
            goto error ;
         }
      }

      if ( _spaceExceedLimit( context, sizeReq ) )
      {
         if ( _overwriteOnExceed( context ) )
         {
            // rc = _recycleOneExtent( context ) ;
            // PD_RC_CHECK( rc, PDERROR,
            //                "Recycle the eldest extent failed[ %d ]", rc ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Size exceed the limit" ) ;
            rc = SDB_OSS_UP_TO_LIMIT ;
            goto error ;
         }
      }

      while ( ( !cb->isInterrupted() ) &&
              ( _numExceedLimit( context, 1 ) ||
                _overwriteOnExceed( context ) ) )
      {
         rc = _popRecord( context, 1, cb, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to pop record, rc: %d", rc ) ;
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

      UINT32 logRecSize = 0 ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
      dpsMergeInfo info ;
      dpsLogRecord &dpsRecord = info.getMergeBlock().record() ;
      CHAR fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
      dmsRecordID rid ;
      UINT64 popCount = 0, popSize = 0 ;

      dmsWriteGuard writeGuard( _service, this, context, cb, TRUE, FALSE, TRUE ) ;

      SDB_ASSERT( context, "context should not be NULL" ) ;
      SDB_ASSERT( cb, "edu cb should not be NULL" ) ;

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

      rc = writeGuard.begin() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to begin write guard, rc: %d", rc ) ;

      if ( dpscb )
      {
         _clFullName( context->mb()->_collectionName, fullName,
                      sizeof(fullName) ) ;
         rc = dpsPop2Record( fullName, logicalID, direction, dpsRecord ) ;
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

      rid.fromUINT64( logicalID ) ;
      rc = context->getCollPtr()->popRecords( rid, direction, cb, popCount, popSize ) ;
      if ( SDB_DMS_RECORD_NOTEXIST == rc )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Failed to pop records from collection, "
                     "record with logical ID [%llu] is not found", logicalID ) ;
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to pop records from collection, rc: %d", rc ) ;

      writeGuard.getPersistGuard().decRecordCount( popCount ) ;
      writeGuard.getPersistGuard().decDataLen( popSize ) ;
      writeGuard.getPersistGuard().decOrgDataLen( popSize ) ;

      if ( dpscb )
      {
         rc = _logDPS( dpscb, info, cb, context, DMS_INVALID_EXTENT,
                       DMS_INVALID_OFFSET, FALSE, DMS_FILE_DATA ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert record into log, rc: %d", rc ) ;
      }
      else if ( cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSN( cb->getEndLsn(), DMS_FILE_DATA ) ;
         cb->setDataExInfo( fullName, this->logicalID(), context->clLID(),
                            DMS_INVALID_EXTENT, DMS_INVALID_OFFSET ) ;
      }

      if ( direction < 0 )
      {
         context->mbStat()->_ridGen.poke( rid.toUINT64() ) ;
      }

      rc = writeGuard.commit() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDSEVERE, "Failed to commit write guard, rc: %d", rc ) ;
         ossPanic() ;
      }

   done:
      if ( 0 != logRecSize)
      {
         pTransCB->releaseLogSpace( logRecSize, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__POPRECORDBYLID, rc ) ;
      return rc ;
   error:
      {
         INT32 rc1 = writeGuard.abort() ;
         if ( SDB_OK != rc1 )
         {
            PD_LOG( PDWARNING, "Failed to abort write guard, rc: %d", rc1 ) ;
         }
      }
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

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         PD_LOG( PDERROR, "Caller must hold mb exclusive lock[%s]",
                 context->toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      while ( number-- > 0 )
      {
         if ( 0 == _mbStatInfo[context->mbID()]._totalRecords.fetch() )
         {
            // No more records
            goto done ;
         }

         rc = _popRecord( context, direction, cb, dpscb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to pop record, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__POPRECORDBYNUMBER, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED__POPRECORD, "_dmsStorageDataCapped::_popRecord" )
   INT32 _dmsStorageDataCapped::_popRecord( dmsMBContext *context,
                                            INT8 direction,
                                            pmdEDUCB *cb,
                                            SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED__POPRECORD ) ;

      dmsRecordID popRID ;
      UINT32 logRecSize = 0 ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
      dpsMergeInfo info ;
      dpsLogRecord &dpsRecord = info.getMergeBlock().record() ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = { 0 } ;
      UINT64 popCount = 0, popSize = 0 ;

      dmsWriteGuard writeGuard( _service, this, context, cb, TRUE, FALSE, TRUE ) ;

      SDB_ASSERT( context, "context should not be NULL" ) ;
      SDB_ASSERT( cb, "edu cb should not be NULL" ) ;

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
      if ( 0 == _mbStatInfo[ context->mbID() ]._totalRecords.fetch() )
      {
         goto done ;
      }

      rc = writeGuard.begin() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to begin write guard, rc: %d", rc ) ;

      if ( direction > 0 )
      {
         rc = context->getCollPtr()->getMinRecordID( popRID, cb ) ;
      }
      else
      {
         rc = context->getCollPtr()->getMaxRecordID( popRID, cb ) ;
      }
      if ( SDB_DMS_EOC == rc )
      {
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get record, rc: %d", rc ) ;

      _clFullName( context->mb()->_collectionName, fullName, sizeof(fullName) ) ;
      if ( dpscb )
      {
         rc = dpsPop2Record( fullName, (INT64)( popRID.toUINT64() ), direction, dpsRecord ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build record, rc: %d", rc ) ;

         rc = dpscb->checkSyncControl( dpsRecord.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc ) ;

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
      else if ( cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSN( cb->getEndLsn(), DMS_FILE_DATA ) ;
         cb->setDataExInfo( fullName, this->logicalID(), context->clLID(),
                            DMS_INVALID_EXTENT, DMS_INVALID_OFFSET ) ;
      }

      rc = context->getCollPtr()->popRecords( popRID, direction, cb, popCount, popSize ) ;
      if ( SDB_DMS_RECORD_NOTEXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to pop record, rc: %d", rc ) ;

      writeGuard.getPersistGuard().decRecordCount( popCount ) ;
      writeGuard.getPersistGuard().decDataLen( popSize ) ;
      writeGuard.getPersistGuard().decOrgDataLen( popSize ) ;

      if ( dpscb )
      {
         rc = _logDPS( dpscb, info, cb, context, popRID._extent, popRID._offset,
                       FALSE, DMS_FILE_DATA ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert record into log, rc: %d", rc ) ;
      }
      else if ( cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSN( cb->getEndLsn(), DMS_FILE_DATA ) ;
         cb->setDataExInfo( fullName, this->logicalID(), context->clLID(),
                            popRID._extent, popRID._offset ) ;
      }

      if ( direction < 0 )
      {
         context->mbStat()->_ridGen.poke( popRID.toUINT64() ) ;
      }

      rc = writeGuard.commit() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDSEVERE, "Failed to commit write guard, rc: %d", rc ) ;
         ossPanic() ;
      }

   done:
      if ( 0 != logRecSize)
      {
         pTransCB->releaseLogSpace( logRecSize, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED__POPRECORD, rc ) ;
      return rc ;

   error:
      {
         INT32 rc1 = writeGuard.abort() ;
         if ( SDB_OK != rc1 )
         {
            PD_LOG( PDWARNING, "Failed to abort write guard, rc: %d", rc1 ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACAPPED_DUMPEXTOPTIONS, "_dmsStorageDataCapped::dumpExtOptions" )
   INT32 _dmsStorageDataCapped::dumpExtOptions( dmsMBContext *context,
                                                BSONObj &extOptions )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACAPPED_DUMPEXTOPTIONS ) ;

      BSONObjBuilder builder ;

      if ( !context->isMBLock() )
      {
         PD_RC_CHECK( rc, PDERROR, "Caller should hold mb shared lock[%s]",
                      context->toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         builder.append( FIELD_NAME_SIZE, context->mb()->_maxSize ) ;
         builder.append( FIELD_NAME_MAX, context->mb()->_maxRecNum ) ;
         builder.appendBool( FIELD_NAME_OVERWRITE, context->mb()->_overwrite ) ;

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

      dmsCappedCLOptions options ;

      PD_CHECK( context->isMBLock( EXCLUSIVE ), SDB_SYS, error, PDERROR,
                "Caller should hold mb exclusive lock [%s]",
                context->toString().c_str() ) ;

      rc = _parseExtendOptions( &extOptions, options ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse options, rc: %d", rc ) ;

      context->mb()->_maxSize = options._maxSize ;
      context->mb()->_maxRecNum = options._maxRecNum ;
      context->mb()->_overwrite = options._overwrite ;

      // on metadata updated
      _onMBUpdated( context->mbID() ) ;

      // Flush MME
      flushMME( isSyncDeep() ) ;

   done :
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACAPPED_SETEXTOPTIONS, rc ) ;
      return rc ;

   error :
      goto done ;
   }
}
