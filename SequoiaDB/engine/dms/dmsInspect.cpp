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

   Source File Name = dmsInspect.cpp

   Descriptive Name = Data Management Service Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/08/2013  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsInspect.hpp"
#include "ixm.hpp"
#include "pmdEDU.hpp"
#include "ixmKey.hpp"
#include "dmsCompress.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "dmsStorageDataCapped.hpp"
#include "dmsStorageLob.hpp"

using namespace bson ;

namespace engine
{

   UINT32 _dmsInspect::inspectHeader( void *inBuf, UINT32 inSize,
                                      CHAR *outBuf, UINT32 outSize,
                                      UINT32 &pageSize, UINT32 &pageNum,
                                      UINT32 &segmentSize,
                                      UINT64 &secretValue, SINT32 &err )
   {
      SDB_ASSERT( outBuf, "outBuf can't be null" ) ;

      SINT32 localErr                    = 0 ;
      UINT32 len                         = 0 ;
      UINT32 dataOffset                  = 0 ;
      dmsStorageUnitHeader *header       = (dmsStorageUnitHeader*)inBuf ;
      CHAR   eyeCatcher [ DMS_HEADER_EYECATCHER_LEN+1 ] = {0} ;

      if ( NULL == inBuf || inSize != DMS_HEADER_SZ )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectHeader input size (%d) doesn't "
                              "match expected size (%d)" OSS_NEWLINE,
                              inSize, DMS_HEADER_SZ ) ;
         ++localErr ;
         goto exit ;
      }

      ossMemcpy ( eyeCatcher, header->_eyeCatcher, DMS_HEADER_EYECATCHER_LEN ) ;
      pageSize = header->_pageSize ;
      pageNum = header->_pageNum ;
      segmentSize = header->_segmentSize ;
      secretValue = header->_secretValue ;

      if ( 0 == segmentSize )
      {
         segmentSize = DMS_SEGMENT_SZ ;
      }

      len += ossSnprintf ( outBuf + len, outSize - len,
                           " Inspect Storage Unit Header: %s" OSS_NEWLINE,
                           header->_name ) ;

      if ( ossStrncmp ( eyeCatcher, DMS_DATASU_EYECATCHER,
                        DMS_HEADER_EYECATCHER_LEN ) == 0
           ||
           ossStrncmp ( eyeCatcher, DMS_DATACAPSU_EYECATCHER,
                        DMS_HEADER_EYECATCHER_LEN ) == 0 )
      {
         dataOffset = DMS_MME_OFFSET + DMS_MME_SZ ;
      }
      else if ( ossStrncmp ( eyeCatcher, DMS_INDEXSU_EYECATCHER,
                             DMS_HEADER_EYECATCHER_LEN ) == 0 )
      {
         dataOffset = DMS_SME_OFFSET + DMS_SME_SZ ;
      }
      else
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid storage unit eye catcher: %s, "
                              "expected: %s" OSS_NEWLINE,
                              eyeCatcher, DMS_DATASU_EYECATCHER ) ;
         ++localErr ;
      }

      /// check pagesize
      if ( pageSize != DMS_PAGE_SIZE4K &&
           pageSize != DMS_PAGE_SIZE8K &&
           pageSize != DMS_PAGE_SIZE16K &&
           pageSize != DMS_PAGE_SIZE32K &&
           pageSize != DMS_PAGE_SIZE64K )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid page size: %d" OSS_NEWLINE,
                              pageSize ) ;
         ++localErr ;
      }

      /// check segmentsize
      if ( !DMS_IS_VALID_SEGMENT( segmentSize ) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid segment size: %d" OSS_NEWLINE,
                              segmentSize ) ;
         ++localErr ;
      }

      if ( header->_storageUnitSize < dataOffset / pageSize )
      {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Storage Unit size is smaller than "
                                 "header: %d" OSS_NEWLINE,
                                 header->_storageUnitSize ) ;
            ++localErr ;
      }

      if ( header->_storageUnitSize - dataOffset / pageSize != (UINT32)pageNum )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Page number is not same with value that "
                              "storage unit size sub header size: %d" OSS_NEWLINE,
                              pageNum ) ;
         ++localErr ;
      }

      if ( ( pageNum % DMS_SEGMENT_PG(segmentSize,pageSize) != 0 ) ||
           ( pageNum > DMS_MAX_PG ) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid page number: %d" OSS_NEWLINE,
                              pageNum ) ;
         ++localErr ;
      }

      if ( header->_numMB > DMS_MME_SLOTS )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid number of collections: %d, "
                              "which should not exceed %d" OSS_NEWLINE,
                              header->_numMB, DMS_MME_SZ/DMS_MB_SIZE ) ;
         ++localErr ;
      }

      if ( header->_numMB > header->_MBHWM )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid number of collections: %d, "
                              "HWM is %d" OSS_NEWLINE,
                              header->_numMB, header->_MBHWM ) ;
         ++localErr ;
      }

   exit :
      if ( 0 == localErr )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Inspect Storage Unit Header Done "
                              "without Error" OSS_NEWLINE ) ;
      }
      else
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Inspect Storage Unit Header Done "
                              "with Error: %d" OSS_NEWLINE, localErr ) ;
      }
      len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;
      err += localErr ;

      return len ;
   }

   #define DMS_INSPECT_SME_STATE_BUFSZ          63

   UINT32 _dmsInspect::inspectSME( void *inBuf, UINT32 inSize,
                                   CHAR *outBuf, UINT32 outSize,
                                   const CHAR *expBuffer, UINT32 pageNum,
                                   SINT32 &hwmPages, SINT32 &err )
   {
      SDB_ASSERT( outBuf, "outBuf can't be null" ) ;

      SINT32 localErr       = 0 ;
      UINT32 len            = 0 ;
      UINT32 totalPages     = 0 ;
      dmsSpaceManagementExtent *pSME = (dmsSpaceManagementExtent*)inBuf ;
      dmsSpaceManagementExtent *pExpSME = (dmsSpaceManagementExtent*)expBuffer ;
      CHAR stateBuf [ DMS_INSPECT_SME_STATE_BUFSZ + 1 ] = {0} ;
      hwmPages              = 0 ;

      if ( NULL == inBuf || inSize != DMS_SME_SZ )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectSME input size (%d) doesn't match "
                              "expected size (%d)" OSS_NEWLINE,
                              inSize,
                              DMS_SME_SZ ) ;
         ++localErr ;
         goto exit ;
      }

      len += ossSnprintf ( outBuf + len, outSize - len,
                           " Inspect Space Management Extent:" OSS_NEWLINE ) ;

      totalPages = DMS_MAX_PG ;

      for ( UINT32 i = 0; i < totalPages ; ++i )
      {
         if ( pExpSME && pExpSME->getBitMask(i) !=
              pSME->getBitMask(i) )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Page State 0x%08x (%d) doesn't "
                                 "match" OSS_NEWLINE,
                                 i, i ) ;
            smeMask2String ( pSME->getBitMask(i), stateBuf,
                             DMS_INSPECT_SME_STATE_BUFSZ ) ;
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "SME State %d (%s)" OSS_NEWLINE,
                                 pSME->getBitMask(i), stateBuf ) ;
            smeMask2String ( pExpSME->getBitMask(i), stateBuf,
                             DMS_INSPECT_SME_STATE_BUFSZ ) ;
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Expected State %d (%s)" OSS_NEWLINE,
                                 pExpSME->getBitMask(i), stateBuf ) ;
            ++localErr ;
         }
         else if ( i < pageNum &&
                   DMS_SME_ALLOCATED == pSME->getBitMask(i) )
         {
            hwmPages = i ;
         }
         else if ( i >= pageNum &&
                   DMS_SME_ALLOCATED == pSME->getBitMask(i) )
         {
            // error
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: allocated page (%d) over page number "
                                 "(%d) " OSS_NEWLINE,
                                 i, pageNum ) ;
            ++localErr ;
         }
      }

   exit :
      if ( 0 == localErr )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Inspect Space Management Extent Done "
                              "without Error" OSS_NEWLINE ) ;
      }
      else
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Inspect Space Management Extent Done "
                              "with Error: %d" OSS_NEWLINE, localErr ) ;
      }
      len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;
      err += localErr ;

      return len ;
   }

   UINT32 _dmsInspect::inspectMME( void *inBuf, UINT32 inSize,
                                   CHAR *outBuf, UINT32 outSize,
                                   const CHAR *pCollectionName,
                                   INT32 maxPages,
                                   vector< UINT16 > &collections,
                                   SINT32 &err )
   {
      SDB_ASSERT( outBuf, "outBuf can't be null" ) ;

      UINT32 len = 0 ;
      SINT32 localErr = 0 ;

      if ( NULL == inBuf || inSize != DMS_MME_SZ )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectMME input size (%d) doesn't match "
                              "expected size (%d)" OSS_NEWLINE,
                              inSize, DMS_MME_SZ ) ;
         ++localErr ;
         goto exit ;
      }

      len += ossSnprintf ( outBuf + len, outSize - len,
                           " Inspect Metadata Management Extent:" OSS_NEWLINE ) ;

      for ( INT32 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         len += inspectMB ( (CHAR*)inBuf + (i*DMS_MB_SIZE), DMS_MB_SIZE,
                            outBuf + len, outSize -len, pCollectionName, i,
                            maxPages, collections, localErr ) ;
         if ( len == outSize )
         {
            goto exit ;
         }
      }

   exit :
      len += ossSnprintf ( outBuf + len, outSize - len,
                           " Inspect Metadata Management Extent Done "
                           "without Error" OSS_NEWLINE ) ;
      len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;
      err += localErr ;

      return len ;
   }

   #define DMS_COLLECTION_STATUS_LEN         127

   UINT32 _dmsInspect::inspectMB( void *inBuf, UINT32 inSize,
                                  CHAR *outBuf, UINT32 outSize,
                                  const CHAR *pCollectionName,
                                  INT32 expCollectionID,
                                  INT32 maxPages,
                                  vector< UINT16 > &collections,
                                  SINT32 &err )
   {
      SDB_ASSERT( outBuf, "outBuf can't be null" ) ;

      SINT32 localErr = 0 ;
      UINT32 len = 0 ;
      dmsMB *mb = (dmsMB*)inBuf ;
      CHAR   tmpStr [ DMS_COLLECTION_STATUS_LEN + 1 ] = {0} ;

      if ( NULL == inBuf || inSize != DMS_MB_SIZE )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectMMEMetadataBlock input size (%d) "
                              "doesn't match expected size (%d)" OSS_NEWLINE,
                              inSize, DMS_MB_SIZE ) ;
         ++localErr ;
         goto exit ;
      }

      // if we want to find a specific collection
      if ( pCollectionName )
      {
         if ( ossStrncmp ( mb->_collectionName, pCollectionName,
                           DMS_COLLECTION_NAME_SZ ) != 0 )
         {
            // if it doesn't match our expectation
            goto exit ;
         }
      }

      if ( DMS_MB_FLAG_FREE != mb->_flag &&
           !OSS_BIT_TEST ( mb->_flag, DMS_MB_FLAG_DROPED ) )
      {
         collections.push_back ( mb->_blockID ) ;

         UINT16 baseFlag = DMS_MB_BASE_FLAG(mb->_flag) ;
         UINT16 oprFlag  = DMS_MB_OPR_FLAG(mb->_flag) ;
         UINT16 phaseFlag = DMS_MB_PHASE_FLAG(mb->_flag) ;

         if ( DMS_MB_FLAG_OFFLINE_REORG == oprFlag )
         {
            oprFlag &= ~DMS_MB_FLAG_OFFLINE_REORG ;

            if ( DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY == phaseFlag )
            {
               phaseFlag &= ~DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY ;
            }
            else if ( DMS_MB_FLAG_OFFLINE_REORG_TRUNCATE == phaseFlag )
            {
               phaseFlag &= ~DMS_MB_FLAG_OFFLINE_REORG_TRUNCATE ;
            }
            else if ( DMS_MB_FLAG_OFFLINE_REORG_COPY_BACK == phaseFlag )
            {
               phaseFlag &= ~DMS_MB_FLAG_OFFLINE_REORG_COPY_BACK ;
            }
            else if ( DMS_MB_FLAG_OFFLINE_REORG_REBUILD == phaseFlag )
            {
               phaseFlag &= ~DMS_MB_FLAG_OFFLINE_REORG_REBUILD ;
            }
         }
         else if ( DMS_MB_FLAG_LOAD == oprFlag )
         {
            oprFlag &= ~DMS_MB_FLAG_LOAD ;

            if ( DMS_MB_FLAG_LOAD_LOAD == phaseFlag )
            {
               phaseFlag &= ~DMS_MB_FLAG_LOAD_LOAD ;
            }
            else if ( DMS_MB_FLAG_LOAD_BUILD == phaseFlag )
            {
               phaseFlag &= ~DMS_MB_FLAG_LOAD_BUILD ;
            }
         }

         if ( DMS_MB_FLAG_USED != baseFlag || 0 != oprFlag || 0 != phaseFlag )
         {
            mbFlag2String ( mb->_flag, tmpStr, DMS_COLLECTION_STATUS_LEN ) ;
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Invalid collection flag: "
                                 "0x%08x (%s)" OSS_NEWLINE,
                                 mb->_flag, tmpStr ) ;
            ++localErr ;
         }

         if ( OSS_BIT_TEST( mb->_attributes, DMS_MB_ATTR_COMPRESSED ) )
         {
            if ( UTIL_COMPRESSOR_LZW != mb->_compressorType &&
                 UTIL_COMPRESSOR_SNAPPY != mb->_compressorType )
            {
               mbAttr2String ( mb->_attributes, tmpStr,
                               DMS_COLLECTION_STATUS_LEN ) ;
               len += ossSnprintf( outBuf + len, outSize - len,
                                   "Error: Imcompatible attribute[0x%08x (%s)] "
                                   "and compressor type[%u]" OSS_NEWLINE,
                                   mb->_attributes, tmpStr,
                                   mb->_compressorType ) ;
               ++localErr ;
            }
         }
         else
         {
            if ( UTIL_COMPRESSOR_INVALID != mb->_compressorType )
            {
               len += ossSnprintf( outBuf + len, outSize - len,
                                   "Error: Imcompatible attribute[0x%08x] "
                                   "and compressor type[%u]" OSS_NEWLINE,
                                   mb->_attributes,
                                   mb->_compressorType ) ;
               ++localErr ;
            }
         }

         if ( mb->_attributes & ~( DMS_MB_ATTR_COMPRESSED |
                                   DMS_MB_ATTR_NOIDINDEX |
                                   DMS_MB_ATTR_CAPPED |
                                   DMS_MB_ATTR_STRICTDATAMODE |
                                   DMS_MB_ATTR_NOTRANS ) )
         {
            mbAttr2String ( mb->_attributes, tmpStr, DMS_COLLECTION_STATUS_LEN ) ;
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Invalid collection attributes: "
                                 "0x%08x (%s)" OSS_NEWLINE,
                                 mb->_attributes, tmpStr ) ;
            ++localErr ;
         }

         if ( mb->_blockID != expCollectionID )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Invalid collection ID: 0x%08lx (%d), "
                                 "expected 0x%08lx (%d)" OSS_NEWLINE,
                                 mb->_blockID, mb->_blockID,
                                 expCollectionID, expCollectionID ) ;
            ++localErr ;
         }
         if ( maxPages >= 0 )
         {
            if ( mb->_firstExtentID != DMS_INVALID_EXTENT &&
                 mb->_firstExtentID > (UINT32)maxPages )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "Error: Invalid first extent: 0x%08lx (%d), "
                                    "max pages: 0x%08lx (%d)" OSS_NEWLINE,
                                    mb->_firstExtentID, mb->_firstExtentID,
                                    maxPages, maxPages ) ;
               ++localErr ;
            }
            if ( mb->_lastExtentID != DMS_INVALID_EXTENT &&
                 mb->_lastExtentID > (UINT32)maxPages )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "Error: Invalid last extent: 0x%08lx (%d), "
                                    "max pages: 0x%08lx (%d)" OSS_NEWLINE,
                                    mb->_lastExtentID, mb->_lastExtentID,
                                    maxPages, maxPages ) ;
               ++localErr ;
            }
         }
         if ( ( mb->_firstExtentID != DMS_INVALID_EXTENT &&
                mb->_lastExtentID == DMS_INVALID_EXTENT ) ||
              ( mb->_firstExtentID == DMS_INVALID_EXTENT &&
                mb->_lastExtentID != DMS_INVALID_EXTENT ) )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Inconsistent first and last extent: "
                                 "0x%08lx (%d) : 0x%08lx (%d)" OSS_NEWLINE,
                                 mb->_firstExtentID, mb->_firstExtentID,
                                 mb->_lastExtentID, mb->_lastExtentID ) ;
            ++localErr ;
         }
         if ( mb->_numIndexes > DMS_COLLECTION_MAX_INDEX )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Num of indexes is out of range: %d"
                                 OSS_NEWLINE,
                                 mb->_numIndexes ) ;
            ++localErr ;
         }
         if ( mb->_numIndexes > mb->_indexHWCount )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Num of indexes %d is greater than HWM: %d"
                                 OSS_NEWLINE,
                                 mb->_numIndexes, mb->_indexHWCount ) ;
            ++localErr ;
         }
         if ( maxPages >= 0 )
         {
            for ( UINT16 i = 0 ; i < dmsMB::_max ; i++ )
            {
               if ( DMS_INVALID_EXTENT != mb->_deleteList[i]._extent &&
                    mb->_deleteList[i]._extent > (UINT32)maxPages )
               {
                  len += ossSnprintf ( outBuf + len, outSize - len,
                                       "Error: Invalid extent for deleteList[%d]: "
                                       "0x%08lx 0x%08lx" OSS_NEWLINE,
                                       i, mb->_deleteList[i]._extent,
                                       mb->_deleteList[i]._offset ) ;
                  ++localErr ;
               }
            }
            /// don't to check this, because the maxPages is data file's,
            /// but is not index files's
            /*
            for ( UINT16 i = 0 ; i < DMS_COLLECTION_MAX_INDEX ; i++ )
            {
               if ( DMS_INVALID_EXTENT != mb->_indexExtent[i] &&
                    mb->_indexExtent[i] >= maxPages )
               {
                  len += ossSnprintf ( outBuf + len, outSize - len,
                                       "Error: Invalid extent for indexCB[%d]: "
                                       "0x%08lx" OSS_NEWLINE,
                                       i, mb->_indexExtent[i] ) ;
                  ++localErr ;
               }
            } */
         }
      }

   exit :
      if ( 0 != localErr )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Inspect Metadata Block [%d] Done "
                              "with Error: %d" OSS_NEWLINE, expCollectionID,
                              localErr ) ;
         len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;
      }
      err += localErr ;

      return len ;
   }

   UINT32 _dmsInspect::inspectExtentHeader( void *inBuf, UINT32 inSize,
                                            CHAR *outBuf, UINT32 outSize,
                                            UINT16 collectionID, SINT32 &err )
   {
      SDB_ASSERT( outBuf, "outBuf can't be null" ) ;

      UINT32 len           = 0 ;
      dmsExtent *extent    = (dmsExtent*)inBuf ;

      if ( NULL == inBuf || inSize < sizeof(dmsExtent) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectExtentHeader input size (%d) "
                              "is too small" OSS_NEWLINE,
                              inSize ) ;
         ++err ;
         goto exit ;
      }

      if ( IXM_EXTENT_CB_EYECATCHER0 == extent->_eyeCatcher[0] &&
                IXM_EXTENT_CB_EYECATCHER1 == extent->_eyeCatcher[1] )
      {
         return inspectIndexCBExtentHeader ( inBuf, inSize, outBuf,
                                             outSize, collectionID, err ) ;
      }
      else
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid eye catcher: %c%c" OSS_NEWLINE,
                              extent->_eyeCatcher[0], extent->_eyeCatcher[1] ) ;
         ++err ;
      }

   exit :
      return len ;
   }

   UINT32 _dmsInspect::inspectIndexCBExtentHeader( void *inBuf, UINT32 inSize,
                                                   CHAR *outBuf, UINT32 outSize,
                                                   UINT16 collectionID,
                                                   SINT32 &err )
   {
      SDB_ASSERT( outBuf, "outBuf can't be null" ) ;

      UINT32 len           = 0 ;
      ixmIndexCBExtent *header = (ixmIndexCBExtent*)inBuf ;

      if ( NULL == inBuf ||
           inSize < sizeof(ixmIndexCBExtent) ||
           inSize % DMS_PAGE_SIZE4K != 0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectIndexCBExtentHeader input size (%d) "
                              "is too small or not aligned with 4K" OSS_NEWLINE,
                              inSize ) ;
         ++err ;
         goto exit ;
      }

      if ( header->_eyeCatcher[0] != IXM_EXTENT_CB_EYECATCHER0 ||
           header->_eyeCatcher[1] != IXM_EXTENT_CB_EYECATCHER1 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid eye catcher: %c%c" OSS_NEWLINE,
                              header->_eyeCatcher[0], header->_eyeCatcher[1] ) ;
         ++err ;
      }

      if ( header->_indexFlag != IXM_INDEX_FLAG_NORMAL &&
           header->_indexFlag != IXM_INDEX_FLAG_CREATING &&
           header->_indexFlag != IXM_INDEX_FLAG_DROPPING &&
           header->_indexFlag != IXM_INDEX_FLAG_INVALID &&
           header->_indexFlag != IXM_INDEX_FLAG_TRUNCATING )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid index flag: %d"
                              OSS_NEWLINE, header->_indexFlag ) ;
         ++err ;
      }
      if ( header->_mbID != collectionID )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Extent ID 0x%08lx (%d) doesn't match "
                              "expected 0x%08lx (%d)" OSS_NEWLINE,
                              header->_mbID, header->_mbID,
                              collectionID, collectionID ) ;
         ++err ;
      }
      if ( header->_flag != DMS_EXTENT_FLAG_INUSE &&
           header->_flag != DMS_EXTENT_FLAG_FREED )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: invalid extent flag 0x%02lx"
                              OSS_NEWLINE, header->_flag ) ;
         ++err ;
      }

      if ( header->_version > DMS_EXTENT_CURRENT_V )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: invalid extent version: 0x%02x, "
                              "current 0x%02x" OSS_NEWLINE,
                              header->_version, DMS_EXTENT_CURRENT_V ) ;
         ++err ;
      }
      try
      {
         BSONObj indexDef ( ((CHAR*)inBuf+sizeof(ixmIndexCBExtent)) ) ;
         if ( !indexDef.isValid() )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: invalid index def" OSS_NEWLINE ) ;
            ++err ;
         }
      }
      catch ( std::exception &e )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Failed to extract index def: %s"
                              OSS_NEWLINE,
                              e.what() ) ;
         ++err ;
      }

   exit :
      return len ;
   }

   UINT32 _dmsInspect::inspectIndexCBExtent( void *inBuf, UINT32 inSize,
                                             CHAR *outBuf, UINT32 outSize,
                                             UINT16 collectionID,
                                             dmsExtentID &root, SINT32 &err )
   {
      SDB_ASSERT( outBuf, "outBuf can't be null" ) ;

      UINT32 len           = 0 ;
      SINT32 localErr      = 0 ;
      ixmIndexCBExtent *extent = (ixmIndexCBExtent*)inBuf ;

      if ( NULL == inBuf ||
           inSize < sizeof(ixmIndexCBExtent) ||
           inSize % DMS_PAGE_SIZE4K != 0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectIndexCBExtent input size (%d) "
                              "is too small or not aligned with 4K" OSS_NEWLINE,
                              inSize ) ;
         ++localErr ;
         goto exit ;
      }

      len += ossSnprintf ( outBuf + len, outSize - len,
                           " Inspect Index Control Block Extent:"
                           OSS_NEWLINE ) ;

      if ( extent->_eyeCatcher[0] != IXM_EXTENT_CB_EYECATCHER0 ||
           extent->_eyeCatcher[1] != IXM_EXTENT_CB_EYECATCHER1 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid eye catcher: %c%c" OSS_NEWLINE,
                              extent->_eyeCatcher[0], extent->_eyeCatcher[1] ) ;
         goto exit ;
      }

      root = extent->_rootExtentID ;

      len += inspectIndexCBExtentHeader ( inBuf, inSize,
                                          outBuf+len, outSize-len,
                                          collectionID, localErr ) ;
   exit :
      if ( 0 == localErr )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Inspect Index Control Block Extent Done "
                              "without Error" OSS_NEWLINE ) ;
      }
      else
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Inspect Index Control Block Extent Done "
                              "with Error: %d" OSS_NEWLINE, localErr ) ;
      }
      len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;
      err += localErr ;

      return len ;
   }

}


