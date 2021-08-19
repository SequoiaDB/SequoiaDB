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
#include "ixmExtent.hpp"
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
      SINT32 localErr                    = 0 ;
      UINT32 len                         = 0 ;
      UINT32 dataOffset                  = 0 ;
      dmsStorageUnitHeader *header       = (dmsStorageUnitHeader*)inBuf ;
      CHAR   eyeCatcher [ DMS_HEADER_EYECATCHER_LEN+1 ] = {0} ;


      if ( NULL == inBuf || NULL == outBuf || inSize != DMS_HEADER_SZ )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectHeader input size (%d) doesn't "
                              "match expected size (%d)"OSS_NEWLINE,
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
                           " Inspect Storage Unit Header: %s"OSS_NEWLINE,
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
                              "expected: %s"OSS_NEWLINE,
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
                              "Error: Invalid page size: %d"OSS_NEWLINE,
                              pageSize ) ;
         ++localErr ;
      }

      /// check segmentsize
      if ( !DMS_IS_VALID_SEGMENT( segmentSize ) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid segment size: %d"OSS_NEWLINE,
                              segmentSize ) ;
         ++localErr ;
      }

      if ( header->_storageUnitSize < dataOffset / pageSize )
      {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Storage Unit size is smaller than "
                                 "header: %d"OSS_NEWLINE,
                                 header->_storageUnitSize ) ;
            ++localErr ;
      }

      if ( header->_storageUnitSize - dataOffset / pageSize != (UINT32)pageNum )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Page number is not same with value that "
                              "storage unit size sub header size: %d"OSS_NEWLINE,
                              pageNum ) ;
         ++localErr ;
      }

      if ( ( pageNum % DMS_SEGMENT_PG(segmentSize,pageSize) != 0 ) ||
           ( pageNum > DMS_MAX_PG ) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid page number: %d"OSS_NEWLINE,
                              pageNum ) ;
         ++localErr ;
      }

      if ( header->_numMB > DMS_MME_SLOTS )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid number of collections: %d, "
                              "which should not exceed %d"OSS_NEWLINE,
                              header->_numMB, DMS_MME_SZ/DMS_MB_SIZE ) ;
         ++localErr ;
      }

      if ( header->_numMB > header->_MBHWM )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid number of collections: %d, "
                              "HWM is %d"OSS_NEWLINE,
                              header->_numMB, header->_MBHWM ) ;
         ++localErr ;
      }

   exit :
      if ( 0 == localErr )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Inspect Storage Unit Header Done "
                              "without Error"OSS_NEWLINE ) ;
      }
      else
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Inspect Storage Unit Header Done "
                              "with Error: %d"OSS_NEWLINE, localErr ) ;
      }
      len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;
      err += localErr ;

      return len ;
   }

   UINT32 _dmsInspect::inspectLobmHeader( void *inBuf, UINT32 inSize,
                                          CHAR *outBuf, UINT32 outSize,
                                          UINT32 sequence, UINT32 &pageNum,
                                          UINT32 &lobmPageSize,
                                          UINT64 secretValue,
                                          INT64 fileSize,
                                          SINT32 &localErr )
   {
      UINT32 len                         = 0 ;
      dmsStorageUnitHeader *header       = (dmsStorageUnitHeader*)inBuf ;
      CHAR   eyeCatcher [ DMS_HEADER_EYECATCHER_LEN+1 ] = {0} ;
      UINT32 segmentSize = 0 ;

      if ( NULL == inBuf || NULL == outBuf || inSize != DMS_HEADER_SZ )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectHeader input size (%d) doesn't "
                              "match expected size (%d)"OSS_NEWLINE,
                              inSize, DMS_HEADER_SZ ) ;
         ++localErr ;
         goto exit ;
      }

      pageNum = header->_pageNum;
      lobmPageSize = header->_pageSize;
      segmentSize = header->_segmentSize ;

      if ( 0 == segmentSize && header->_lobdPageSize > 0 )
      {
         segmentSize = DMS_SEGMENT_SZ / header->_lobdPageSize * lobmPageSize ;
      }

      ossMemcpy ( eyeCatcher, header->_eyeCatcher, DMS_HEADER_EYECATCHER_LEN ) ;

      len += ossSnprintf ( outBuf + len, outSize - len,
                           "Inspect Storage Unit Header: %s"OSS_NEWLINE,
                           header->_name ) ;

      if ( ossStrncmp ( eyeCatcher, DMS_LOBM_EYECATCHER,
                        DMS_HEADER_EYECATCHER_LEN ) != 0)
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid storage unit eye catcher: %s, "
                              "expected: %s"OSS_NEWLINE,
                              eyeCatcher, DMS_LOBM_EYECATCHER ) ;
         ++localErr ;
      }


      if ( header->_version > DMS_LOB_CUR_VERSION )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid lob version: %d"OSS_NEWLINE,
                              header->_version ) ;
         ++localErr ;
      }

      if ( header->_pageSize != DMS_PAGE_SIZE256B &&
           header->_pageSize != DMS_PAGE_SIZE64B)
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid page size: %d"OSS_NEWLINE,
                              header->_pageSize ) ;
         ++localErr ;
      }

      if ( header->_storageUnitSize !=  fileSize / header->_pageSize )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Storage Unit size is smaller than "
                              "header: %d"OSS_NEWLINE,
                              header->_storageUnitSize ) ;
         ++localErr ;
      }

      if ( ( segmentSize > 0 &&
             pageNum % DMS_SEGMENT_PG( segmentSize, lobmPageSize ) != 0 ) ||
           header->_pageNum > DMS_MAX_PG )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid page number: %d"OSS_NEWLINE,
                              header->_pageNum ) ;
         ++localErr ;
      }
      pageNum =  header->_pageNum;

      // check
      if ( header->_secretValue != secretValue )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Secret value[%llu] is not expected[%llu]"
                              OSS_NEWLINE,
                              header->_secretValue , secretValue ) ;
         ++localErr ;
      }

      if ( header->_sequence != sequence )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: sequence value[%llu] is not "
                              "expected[%llu]"OSS_NEWLINE,
                              header->_sequence , sequence ) ;
         ++localErr ;
      }

      if ( header->_numMB  != 0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid number of collections: %d, "
                              "which should be %d"OSS_NEWLINE,
                              header->_numMB, 0) ;
         ++localErr ;
      }

      if ( header->_numMB !=  header->_MBHWM )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid number of collections: %d, "
                              "HWM is %d"OSS_NEWLINE,
                              header->_numMB, header->_MBHWM ) ;
         ++localErr ;
      }

   exit :
      if ( 0 == localErr )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Inspect Storage Unit Header Done "
                              "without Error"OSS_NEWLINE ) ;
      }
      else
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Inspect Storage Unit Header Done "
                              "with Error: %d"OSS_NEWLINE, localErr ) ;
      }
      len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;

      return len ;
   }

   UINT32 _dmsInspect::inspectLobdHeader( void *inBuf, UINT32 inSize,
                                          CHAR *outBuf, UINT32 outSize,
                                          UINT32 sequence, UINT64 secretValue,
                                          INT64 fileSize, INT32 &totalErr)
   {
      SINT32 localErr                    = 0 ;
      UINT32 len                         = 0 ;
      dmsStorageUnitHeader *header       = (dmsStorageUnitHeader*)inBuf ;
      CHAR   eyeCatcher [ DMS_HEADER_EYECATCHER_LEN+1 ] = {0} ;

      if ( NULL == inBuf || NULL == outBuf || inSize != DMS_HEADER_SZ )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectHeader input size (%d) doesn't "
                              "match expected size (%d)"OSS_NEWLINE,
                              inSize, DMS_HEADER_SZ ) ;
         ++localErr ;
      }

      ossMemcpy ( eyeCatcher, header->_eyeCatcher, DMS_HEADER_EYECATCHER_LEN ) ;

      len += ossSnprintf ( outBuf + len, outSize - len,
                           "Inspect Storage Unit Header: %s"OSS_NEWLINE,
                           header->_name ) ;

      if ( ossStrncmp ( eyeCatcher, DMS_LOBD_EYECATCHER,
                 DMS_HEADER_EYECATCHER_LEN ) != 0)
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid storage unit eye catcher: %s, "
                              "expected: %s"OSS_NEWLINE,
                              eyeCatcher, DMS_LOBD_EYECATCHER ) ;
         ++localErr ;
      }


      if ( header->_version > DMS_LOB_CUR_VERSION )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid lob version: %d"OSS_NEWLINE,
                              header->_version ) ;
         ++localErr ;
      }

      if ( header->_pageSize != 0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid page size: %d"OSS_NEWLINE,
                              header->_pageSize ) ;
         ++localErr ;
      }

      if ( header->_storageUnitSize !=  0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Storage Unit size should not be: %d"
                              OSS_NEWLINE,
                              header->_storageUnitSize) ;
         ++localErr ;
      }

      if ( header->_pageNum != 0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: page number should not be: %d"OSS_NEWLINE,
                              header->_pageNum );
         ++localErr ;
      }

      // check
      if ( header->_secretValue  != secretValue )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Secret value[%llu] is not expected[%llu]"
                              OSS_NEWLINE,
                              header->_secretValue , secretValue ) ;
         ++localErr ;
      }

      if ( header->_sequence != sequence )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                             "Error: sequence value[%llu] is not expected[%llu]"
                             OSS_NEWLINE,
                             header->_sequence , sequence ) ;
         ++localErr ;
      }


      if ( header->_numMB  != 0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid number of collections: %d, "
                              "which should be: %d"OSS_NEWLINE,
                              header->_numMB, DMS_MME_SZ/DMS_MB_SIZE ) ;
         ++localErr ;
      }

      if ( header->_numMB !=  header->_MBHWM )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid number of collections: %d, "
                              "HWM is %d"OSS_NEWLINE,
                              header->_numMB, header->_MBHWM ) ;
         ++localErr ;
      }

      if ( 0 == localErr )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Inspect Storage Unit Header Done "
                              "without Error"OSS_NEWLINE ) ;
      }
      else
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Inspect Storage Unit Header Done "
                              "with Error: %d"OSS_NEWLINE, localErr ) ;
      }
      len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;

      totalErr += localErr;
      return len ;
   }

   #define DMS_INSPECT_SME_STATE_BUFSZ          63

   UINT32 _dmsInspect::inspectSME( void *inBuf, UINT32 inSize,
                                   CHAR *outBuf, UINT32 outSize,
                                   const CHAR *expBuffer, UINT32 pageNum,
                                   SINT32 &hwmPages, SINT32 &err )
   {
      SINT32 localErr       = 0 ;
      UINT32 len            = 0 ;
      UINT32 totalPages     = 0 ;
      dmsSpaceManagementExtent *pSME = (dmsSpaceManagementExtent*)inBuf ;
      dmsSpaceManagementExtent *pExpSME = (dmsSpaceManagementExtent*)expBuffer ;
      CHAR stateBuf [ DMS_INSPECT_SME_STATE_BUFSZ + 1 ] = {0} ;
      hwmPages              = 0 ;

      if ( NULL == inBuf || NULL == outBuf || inSize != DMS_SME_SZ )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectSME input size (%d) doesn't match "
                              "expected size (%d)"OSS_NEWLINE,
                              inSize,
                              DMS_SME_SZ ) ;
         ++localErr ;
         goto exit ;
      }

      len += ossSnprintf ( outBuf + len, outSize - len,
                           " Inspect Space Management Extent:"OSS_NEWLINE ) ;

      totalPages = DMS_MAX_PG ;

      for ( UINT32 i = 0; i < totalPages ; ++i )
      {
         if ( pExpSME && pExpSME->getBitMask(i) !=
              pSME->getBitMask(i) )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Page State 0x%08x (%d) doesn't "
                                 "match"OSS_NEWLINE,
                                 i, i ) ;
            smeMask2String ( pSME->getBitMask(i), stateBuf,
                             DMS_INSPECT_SME_STATE_BUFSZ ) ;
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "SME State %d (%s)"OSS_NEWLINE,
                                 pSME->getBitMask(i), stateBuf ) ;
            smeMask2String ( pExpSME->getBitMask(i), stateBuf,
                             DMS_INSPECT_SME_STATE_BUFSZ ) ;
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Expected State %d (%s)"OSS_NEWLINE,
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
                                 "(%d) "OSS_NEWLINE,
                                 i, pageNum ) ;
            ++localErr ;
         }
      }

   exit :
      if ( 0 == localErr )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Inspect Space Management Extent Done "
                              "without Error"OSS_NEWLINE ) ;
      }
      else
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Inspect Space Management Extent Done "
                              "with Error: %d"OSS_NEWLINE, localErr ) ;
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
      UINT32 len = 0 ;
      SINT32 localErr = 0 ;

      if ( NULL == inBuf || NULL == outBuf || inSize != DMS_MME_SZ )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectMME input size (%d) doesn't match "
                              "expected size (%d)"OSS_NEWLINE,
                              inSize, DMS_MME_SZ ) ;
         ++localErr ;
         goto exit ;
      }

      len += ossSnprintf ( outBuf + len, outSize - len,
                           " Inspect Metadata Management Extent:"OSS_NEWLINE ) ;

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
                           "without Error"OSS_NEWLINE ) ;
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
      SINT32 localErr = 0 ;
      UINT32 len = 0 ;
      dmsMB *mb = (dmsMB*)inBuf ;
      CHAR   tmpStr [ DMS_COLLECTION_STATUS_LEN + 1 ] = {0} ;

      if ( NULL == inBuf || NULL == outBuf || inSize != DMS_MB_SIZE )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectMMEMetadataBlock input size (%d) "
                              "doesn't match expected size (%d)"OSS_NEWLINE,
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
                                 "0x%08x (%s)"OSS_NEWLINE,
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
                                   "and compressor type[%u]"OSS_NEWLINE,
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
                                   "and compressor type[%u]"OSS_NEWLINE,
                                   mb->_attributes,
                                   mb->_compressorType ) ;
               ++localErr ;
            }
         }

         if ( mb->_attributes & ~(DMS_MB_ATTR_COMPRESSED|DMS_MB_ATTR_NOIDINDEX
                                  |DMS_MB_ATTR_CAPPED) )
         {
            mbAttr2String ( mb->_attributes, tmpStr, DMS_COLLECTION_STATUS_LEN ) ;
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Invalid collection attributes: "
                                 "0x%08x (%s)"OSS_NEWLINE,
                                 mb->_attributes, tmpStr ) ;
            ++localErr ;
         }

         if ( mb->_blockID != expCollectionID )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Invalid collection ID: 0x%08lx (%d), "
                                 "expected 0x%08lx (%d)"OSS_NEWLINE,
                                 mb->_blockID, mb->_blockID,
                                 expCollectionID, expCollectionID ) ;
            ++localErr ;
         }
         if ( maxPages >= 0 )
         {
            if ( mb->_firstExtentID != DMS_INVALID_EXTENT &&
                 mb->_firstExtentID > maxPages )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "Error: Invalid first extent: 0x%08lx (%d), "
                                    "max pages: 0x%08lx (%d)"OSS_NEWLINE,
                                    mb->_firstExtentID, mb->_firstExtentID,
                                    maxPages, maxPages ) ;
               ++localErr ;
            }
            if ( mb->_lastExtentID != DMS_INVALID_EXTENT &&
                 mb->_lastExtentID > maxPages )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "Error: Invalid last extent: 0x%08lx (%d), "
                                    "max pages: 0x%08lx (%d)"OSS_NEWLINE,
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
                                 "0x%08lx (%d) : 0x%08lx (%d)"OSS_NEWLINE,
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
                    mb->_deleteList[i]._extent > maxPages )
               {
                  len += ossSnprintf ( outBuf + len, outSize - len,
                                       "Error: Invalid extent for deleteList[%d]: "
                                       "0x%08lx 0x%08lx"OSS_NEWLINE,
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
                                       "0x%08lx"OSS_NEWLINE,
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
                              "with Error: %d"OSS_NEWLINE, expCollectionID,
                              localErr ) ;
         len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;
      }
      err += localErr ;

      return len ;
   }

   UINT32 _dmsInspect::inspectDataExtent( pmdEDUCB *cb, CHAR *inBuf,
                                          UINT32 inSize, CHAR *outBuf,
                                          UINT32 outSize, INT32 maxPages,
                                          UINT16 collectionID,
                                          dmsExtentID &nextExtent,
                                          set< dmsRecordID > *ridList,
                                          SINT32 &err,
                                          dmsCompressorEntry *compressorEntry,
                                          UINT64 &recordNum,
                                          UINT64 &compressedNum,
                                          BOOLEAN capped )
   {
      UINT32 len           = 0 ;
      SINT32 localErr      = 0 ;
      dmsExtent *extent    = (dmsExtent*)inBuf ;
      dmsExtentID origID   = nextExtent ;

      if ( NULL == inBuf || NULL == outBuf || inSize < sizeof(dmsExtent) ||
           inSize % DMS_PAGE_SIZE4K != 0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectDataExtent input size (%d) "
                              "is too small or not aligned with 4K"OSS_NEWLINE,
                              inSize ) ;
         nextExtent = DMS_INVALID_EXTENT ;
         ++localErr ;
         goto exit ;
      }

      if ( extent->_eyeCatcher[0] != DMS_EXTENT_EYECATCHER0 ||
           extent->_eyeCatcher[1] != DMS_EXTENT_EYECATCHER1 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
                              extent->_eyeCatcher[0], extent->_eyeCatcher[1] ) ;
         nextExtent = DMS_INVALID_EXTENT ;
         ++localErr ;
      }
      if ( DMS_INVALID_EXTENT != nextExtent && nextExtent > maxPages )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Next extent is out of range: "
                              "0x%08x (%d)"OSS_NEWLINE,
                              nextExtent, nextExtent ) ;
         ++localErr ;
      }
      nextExtent = extent->_nextExtent ;

      if ( capped )
      {
         len += inspectCappedExtent( inBuf, inSize, outBuf + len, outSize - len,
                                     collectionID, compressorEntry, recordNum,
                                     compressedNum, localErr, cb ) ;
      }
      else
      {
         len += inspectNormalExtent( inBuf, inSize, outBuf + len, outSize - len,
                                     collectionID, compressorEntry, recordNum,
                                     compressedNum, localErr, ridList, cb ) ;
      }

   exit :
      if ( 0 != localErr )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Inspect Data Extent 0x%08lx (%d) Done "
                              "with Error: %d"OSS_NEWLINE, origID,
                              origID, localErr ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              OSS_NEWLINE ) ;
      }
      err += localErr ;

      return len ;
   }

   UINT32 _dmsInspect::inspectDataRecord( pmdEDUCB *cb, void *inBuf,
                                          UINT32 inSize, CHAR *outBuf,
                                          UINT32 outSize, INT32 currentRecordID,
                                          dmsOffset &nextRecord,
                                          set< dmsRecordID > *ridList,
                                          SINT32 &err,
                                          dmsCompressorEntry *compressorEntry,
                                          BOOLEAN &isCompressed )
   {
      INT32 rc          = SDB_OK ;
      UINT32 len        = 0 ;
      dmsRecord *record = (dmsRecord*)inBuf ;
      CHAR flag         = 0 ;
      CHAR state        = 0 ;
      BOOLEAN   isDel   = FALSE ;
      BOOLEAN   isOvf   = FALSE ;

      if ( NULL == inBuf  || NULL == outBuf || inSize < sizeof(dmsRecord) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectDataRecord input size (%d) "
                              "is too small"OSS_NEWLINE,
                              inSize ) ;
         nextRecord = DMS_INVALID_OFFSET ;
         ++err ;
         goto exit ;
      }

      flag       = record->getFlag() ;
      state      = record->getState();

      if ( flag & DMS_RECORD_FLAG_COMPRESSED )
      {
         isCompressed = TRUE ;
      }
      else
      {
         isCompressed = FALSE ;
      }

      if ( OSS_BIT_TEST ( flag, DMS_RECORD_FLAG_OVERFLOWF ) )
      {
         isOvf = TRUE ;
      }

      if ( OSS_BIT_TEST ( flag, DMS_RECORD_FLAG_DELETED ) )
      {
         isDel = TRUE ;
      }

      if ( OSS_BIT_CLEAR ( state, DMS_RECORD_FLAG_OVERFLOWF |
                                  DMS_RECORD_FLAG_OVERFLOWT |
                                  DMS_RECORD_FLAG_DELETED ) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Unexpected flag (0x%02x) for %dth record"
                              OSS_NEWLINE,
                              flag, currentRecordID ) ;
         ++err ;
      }
      if ( DMS_INVALID_OFFSET != nextRecord && record->_myOffset != nextRecord )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: my offset (0x%08x) does not match "
                              "expected ( 0x%08x)"OSS_NEWLINE,
                              record->_myOffset, nextRecord ) ;
         nextRecord = DMS_INVALID_OFFSET ;
         ++err ;
      }
      else
      {
         nextRecord = record->_nextOffset ;
      }

      if ( isDel )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Deleted record is detected (0x%08x)"
                              OSS_NEWLINE, nextRecord ) ;
         ++err ;
         nextRecord = DMS_INVALID_OFFSET ;
      }
      else if ( isOvf )
      {
         dmsRecordID rid = record->getOvfRID() ;
         if ( ridList )
         {
            ridList->insert ( rid ) ;
         }
      }
      else
      {
         // for normal and ovfto types, let's inspect data
         try
         {
            /// first to inc error
            ++err ;

            ossValuePtr recordPtr = 0 ;
            DMS_RECORD_EXTRACTDATA ( record, recordPtr,
                                     compressorEntry ) ;
            BSONObj obj ( (CHAR*)recordPtr ) ;
            if ( !obj.isValid() )
            {
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "Error: Detected invalid record (0x%08x)"
                                    OSS_NEWLINE, nextRecord ) ;
            }
            /// dec error
            else
            {
               --err ;
            }
         }
         catch ( std::exception &e )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Failed to format "
                                 "record: %s"OSS_NEWLINE,
                                 e.what() ) ;
         }
      }

   exit :
      return len ;
   error:
      goto exit ;
   }

   UINT32 _dmsInspect::inspectCappedDataRecord( pmdEDUCB *cb,
                                                dmsCappedRecord *record,
                                                CHAR *outBuf,
                                                UINT32 outSize,
                                                dmsOffset currentOffset,
                                                SINT32 &err,
                                                dmsCompressorEntry *compressorEntry )
   {
      INT32 rc = SDB_OK ;
      UINT32 len = 0 ;

      try
      {
         /// first to inc error
         ++err ;

         ossValuePtr recordPtr = 0 ;
         DMS_RECORD_EXTRACTDATA ( record, recordPtr,
                                  compressorEntry ) ;
         BSONObj obj ( (CHAR*)recordPtr ) ;
         if ( !obj.isValid() )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Detected invalid record (0x%08x)"
                                 OSS_NEWLINE, currentOffset ) ;
         }
         /// dec error
         else
         {
            --err ;
         }
      }
      catch ( std::exception &e )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Failed to format "
                              "record: %s"OSS_NEWLINE,
                              e.what() ) ;
      }

   exit :
      return len ;
   error:
      goto exit ;

   }

   UINT32 _dmsInspect::inspectExtentHeader( void *inBuf, UINT32 inSize,
                                            CHAR *outBuf, UINT32 outSize,
                                            UINT16 collectionID, SINT32 &err )
   {
      UINT32 len           = 0 ;
      dmsExtent *extent    = (dmsExtent*)inBuf ;

      if ( NULL == inBuf || NULL == outBuf || inSize < sizeof(dmsExtent) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectExtentHeader input size (%d) "
                              "is too small"OSS_NEWLINE,
                              inSize ) ;
         ++err ;
         goto exit ;
      }

      if ( DMS_EXTENT_EYECATCHER0 == extent->_eyeCatcher[0] &&
           DMS_EXTENT_EYECATCHER1 == extent->_eyeCatcher[1] )
      {
         return inspectDataExtentHeader ( inBuf, inSize, outBuf,
                                          outSize, collectionID, err ) ;
      }
      else if ( IXM_EXTENT_EYECATCHER0 == extent->_eyeCatcher[0] &&
                IXM_EXTENT_EYECATCHER1 == extent->_eyeCatcher[1] )
      {
         dmsExtentID dummyParent ;
         return inspectIndexExtentHeader ( inBuf, inSize, outBuf,
                                           outSize, collectionID,
                                           dummyParent, err ) ;
      }
      else if ( IXM_EXTENT_CB_EYECATCHER0 == extent->_eyeCatcher[0] &&
                IXM_EXTENT_CB_EYECATCHER1 == extent->_eyeCatcher[1] )
      {
         return inspectIndexCBExtentHeader ( inBuf, inSize, outBuf,
                                             outSize, collectionID, err ) ;
      }
      else
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
                              extent->_eyeCatcher[0], extent->_eyeCatcher[1] ) ;
         ++err ;
      }

   exit :
      return len ;
   }

   UINT32 _dmsInspect::inspectDataExtentHeader( void *inBuf, UINT32 inSize,
                                                CHAR *outBuf, UINT32 outSize,
                                                UINT16 collectionID,
                                                SINT32 &err )
   {
      UINT32 len           = 0 ;
      dmsExtent *extent    = (dmsExtent*)inBuf ;

      if ( NULL == inBuf || NULL == outBuf || inSize < sizeof(dmsExtent) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectExtentHeader input size (%d) "
                              "is too small"OSS_NEWLINE,
                              inSize ) ;
         ++err ;
         goto exit ;
      }

      if ( DMS_EXTENT_EYECATCHER0 != extent->_eyeCatcher[0] ||
           DMS_EXTENT_EYECATCHER1 != extent->_eyeCatcher[1] )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
                              extent->_eyeCatcher[0], extent->_eyeCatcher[1] ) ;
         ++err ;
      }
      if ( extent->_mbID != collectionID )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Extent ID 0x%08lx (%d) doesn't match "
                              "expected 0x%08lx (%d)"OSS_NEWLINE,
                              extent->_mbID, extent->_mbID,
                              collectionID, collectionID ) ;
         ++err ;
      }
      if ( extent->_flag != DMS_EXTENT_FLAG_INUSE &&
           extent->_flag != DMS_EXTENT_FLAG_FREED )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: invalid extent flag 0x%02lx"
                              OSS_NEWLINE, extent->_flag ) ;
         ++err ;
      }
      if ( extent->_version > DMS_EXTENT_CURRENT_V )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: invalid extent version: 0x%02x, "
                              "current 0x%02x"OSS_NEWLINE,
                              extent->_version, DMS_EXTENT_CURRENT_V ) ;
         ++err ;
      }
      if ( ( extent->_firstRecordOffset == DMS_INVALID_OFFSET &&
             extent->_lastRecordOffset != DMS_INVALID_OFFSET ) ||
           ( extent->_firstRecordOffset != DMS_INVALID_OFFSET &&
             extent->_lastRecordOffset == DMS_INVALID_OFFSET ) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: first and last record offset are "
                              "inconsistent: 0x%08lx (%d), 0x%08lx (%d)"
                              OSS_NEWLINE,
                              extent->_firstRecordOffset,
                              extent->_firstRecordOffset,
                              extent->_lastRecordOffset,
                              extent->_lastRecordOffset ) ;
         ++err ;
      }

   exit :
      return len ;
   }

   UINT32 _dmsInspect::inspectIndexRecord( void *inBuf, UINT32 inSize,
                                           CHAR *outBuf, UINT32 outSize,
                                           UINT32 keyOffset, SINT32 &err )
   {
      UINT32 len = 0 ;
      dmsRecordID rid ;
      UINT16 keyOfst ;
      ixmKeyNode *keyNode = NULL ;

      if ( keyOffset > inSize )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: key offset is out of range: %u"
                              OSS_NEWLINE, keyOffset ) ;
         ++err ;
         goto exit ;
      }

      keyNode   = (ixmKeyNode*)(((CHAR*)inBuf)+keyOffset ) ;
      rid       = keyNode->_rid ;
      keyOfst   = keyNode->_keyOffset ;

      if ( keyOfst > inSize )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Key Offset is out of range: %u"
                              OSS_NEWLINE, keyOfst ) ;
         ++err ;
      }
      try
      {
         ixmKey key ( ((CHAR*)inBuf)+keyOfst ) ;
         if ( !key.isValid() )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Key is not valid: %u"
                                 OSS_NEWLINE, keyOfst ) ;
            ++err ;
         }
      }
      catch ( std::exception &e )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Failed to extract key: %s"
                              OSS_NEWLINE, e.what() ) ;
         ++err ;
      }

   exit :
      return len ;
   }

   UINT32 _dmsInspect::inspectIndexExtentHeader( void *inBuf, UINT32 inSize,
                                                 CHAR *outBuf, UINT32 outSize,
                                                 UINT16 collectionID,
                                                 dmsExtentID &parentExtent,
                                                 SINT32 &err )
   {
      UINT32 len           = 0 ;
      ixmExtentHead *header=(ixmExtentHead*)inBuf ;

      if ( NULL == inBuf || NULL == outBuf || inSize < sizeof(ixmExtentHead) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectIndexExtentHeader input size (%d) "
                              "is too small"OSS_NEWLINE,
                              inSize ) ;
         ++err ;
         goto exit ;
      }

      parentExtent = header->_parentExtentID ;

      if ( IXM_EXTENT_EYECATCHER0 != header->_eyeCatcher[0] ||
           IXM_EXTENT_EYECATCHER1 != header->_eyeCatcher[1] )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
                              header->_eyeCatcher[0], header->_eyeCatcher[1] ) ;
         ++err ;
      }
      if ( header->_mbID != collectionID )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Extent ID 0x%08lx (%d) doesn't match "
                              "expected 0x%08lx (%d)"OSS_NEWLINE,
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
                              "current 0x%02x"OSS_NEWLINE,
                              header->_version, DMS_EXTENT_CURRENT_V ) ;
         ++err ;
      }

      if ( header->_beginFreeOffset > inSize )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: beginFreeOffset is out of range: "
                              "0x%08lx (%d), inSize 0x%08lx (%d)"OSS_NEWLINE,
                              header->_beginFreeOffset,
                              header->_beginFreeOffset,
                              inSize, inSize ) ;
         ++err ;
      }
      if ( header->_totalFreeSize > inSize - sizeof(ixmExtentHead) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: totalFreeSize is out of range: "
                              "0x%08lx (%d), inSize 0x%08lx (%d)"OSS_NEWLINE,
                              header->_totalFreeSize, header->_totalFreeSize,
                              inSize, inSize ) ;
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
      UINT32 len           = 0 ;
      ixmIndexCBExtent *header = (ixmIndexCBExtent*)inBuf ;

      if ( NULL == inBuf || NULL == outBuf ||
           inSize < sizeof(ixmIndexCBExtent) ||
           inSize % DMS_PAGE_SIZE4K != 0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectIndexCBExtentHeader input size (%d) "
                              "is too small or not aligned with 4K"OSS_NEWLINE,
                              inSize ) ;
         ++err ;
         goto exit ;
      }

      if ( header->_eyeCatcher[0] != IXM_EXTENT_CB_EYECATCHER0 ||
           header->_eyeCatcher[1] != IXM_EXTENT_CB_EYECATCHER1 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
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
                              "expected 0x%08lx (%d)"OSS_NEWLINE,
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
                              "current 0x%02x"OSS_NEWLINE,
                              header->_version, DMS_EXTENT_CURRENT_V ) ;
         ++err ;
      }
      try
      {
         BSONObj indexDef ( ((CHAR*)inBuf+sizeof(ixmIndexCBExtent)) ) ;
         if ( !indexDef.isValid() )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: invalid index def"OSS_NEWLINE ) ;
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
      UINT32 len           = 0 ;
      SINT32 localErr      = 0 ;
      ixmIndexCBExtent *extent = (ixmIndexCBExtent*)inBuf ;

      if ( NULL == inBuf || NULL == outBuf ||
           inSize < sizeof(ixmIndexCBExtent) ||
           inSize % DMS_PAGE_SIZE4K != 0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectIndexCBExtent input size (%d) "
                              "is too small or not aligned with 4K"OSS_NEWLINE,
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
                              "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
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
                              "without Error"OSS_NEWLINE ) ;
      }
      else
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Inspect Index Control Block Extent Done "
                              "with Error: %d"OSS_NEWLINE, localErr ) ;
      }
      len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;
      err += localErr ;

      return len ;
   }

   UINT32 _dmsInspect::inspectIndexExtent( pmdEDUCB *cb, void *inBuf,
                                           UINT32 inSize, CHAR *outBuf,
                                           UINT32 outSize, UINT16 collectionID,
                                           dmsExtentID extentID,
                                           deque< dmsExtentID > &childExtents,
                                           SINT32 &err )
   {
      UINT32 len           = 0 ;
      SINT32 localErr      = 0 ;
      ixmExtentHead *extentHead = (ixmExtentHead*)inBuf ;

      if ( NULL == inBuf || NULL == outBuf || inSize < sizeof(ixmExtentHead) ||
           inSize % DMS_PAGE_SIZE4K != 0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: inspectIndexExtent input size (%d) "
                              "is too small or not aligned with 4K"OSS_NEWLINE,
                              inSize ) ;
         ++localErr ;
         goto exit ;
      }

      if ( extentHead->_eyeCatcher[0] != IXM_EXTENT_EYECATCHER0 ||
           extentHead->_eyeCatcher[1] != IXM_EXTENT_EYECATCHER1 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
                              extentHead->_eyeCatcher[0],
                              extentHead->_eyeCatcher[1] ) ;
         goto exit ;
      }

      // get all child extents
      for ( INT32 i = 0; i < extentHead->_totalKeyNodeNum; ++i )
      {
         UINT32 keyOffset = sizeof(ixmExtentHead) +
                            sizeof(ixmKeyNode)*i ;
         ixmKeyNode *key = (ixmKeyNode*)(((CHAR*)inBuf)+keyOffset) ;
         if ( keyOffset > inSize )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: key offset is out of range: %d, "
                                 "extent size: %d, key pos: %d"OSS_NEWLINE,
                                 keyOffset, inSize, i ) ;
            ++localErr ;
            goto exit ;
         }
         if ( key->_left != DMS_INVALID_EXTENT )
         {
            childExtents.push_back ( key->_left ) ;
         }
      }
      if ( extentHead->_right != DMS_INVALID_EXTENT )
      {
         childExtents.push_back ( extentHead->_right ) ;
      }

      len += inspectExtentHeader ( inBuf, inSize, outBuf + len,
                                   outSize - len, collectionID,
                                   localErr ) ;

      if ( DMS_EXTENT_FLAG_FREED == extentHead->_flag )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Extent is not in use"OSS_NEWLINE ) ;
         ++localErr ;
         goto exit ;
      }
      // inspect all index keys
      for ( INT32 i = 0 ; i < extentHead->_totalKeyNodeNum ; ++i )
      {
         UINT32 keyOffset = sizeof(ixmExtentHead) +
                            sizeof(ixmKeyNode)*i ;
         if ( keyOffset > inSize )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: key offset is out of range: %d, "
                                 "extent size: %d, key pos: %d"OSS_NEWLINE,
                                 keyOffset, inSize, i ) ;
            ++localErr ;
            continue ;
         }
         len += inspectIndexRecord ( ((CHAR*)inBuf),
                                     inSize,
                                     outBuf + len, outSize - len,
                                     keyOffset, localErr ) ;
      } // for ( INT32 i = 0; i < extentHead->_totalKeyNodeNum; ++i )

   exit :
      if ( 0 != localErr )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Inspect Index Extent 0x%08lx (%d) Done "
                              "with Error: %d"OSS_NEWLINE,
                              extentID, extentID, localErr ) ;
         len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;
      }
      err += localErr ;

      return len ;
   }

   INT32 _dmsInspect::inspectNormalExtent( CHAR *inBuf, UINT32 inSize,
                                           CHAR *outBuf, UINT32 outSize,
                                           UINT16 collectionID,
                                           dmsCompressorEntry *compressorEntry,
                                           UINT64 &recordNum,
                                           UINT64 &compressedNum,
                                           INT32 &localErr,
                                           set< dmsRecordID > *ridList,
                                           pmdEDUCB *cb )
   {
      UINT32 len = 0 ;
      INT32 recordCount = 0 ;
      dmsExtent *extent = (dmsExtent *)inBuf ;
      BOOLEAN isCompressed = FALSE ;

      len += inspectExtentHeader ( inBuf, inSize, outBuf + len,
                                   outSize - len, collectionID, localErr ) ;
      // make sure the extent is valid and in use
      if ( DMS_EXTENT_FLAG_FREED == extent->_flag )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Extent is not in use"OSS_NEWLINE ) ;
         ++localErr ;
      }
      // start inspect all records
      dmsOffset nextRecord = extent->_firstRecordOffset ;
      while ( DMS_INVALID_OFFSET != nextRecord && len < outSize )
      {
         if ( nextRecord >= (SINT32)inSize )
         {
            len += ossSnprintf (  outBuf + len, outSize - len,
                                  "Error : nextRecord %d is greater "
                                  "than inSize %d",
                                  nextRecord, inSize ) ;
            ++localErr ;
         }

         len += inspectDataRecord ( cb, ((CHAR*)inBuf)+nextRecord,
                                    inSize - nextRecord,
                                    outBuf + len, outSize - len,
                                    recordCount, nextRecord,
                                    ridList, localErr,
                                    compressorEntry,
                                    isCompressed ) ;
         ++recordCount ;
         ++recordNum ;
         if ( isCompressed )
         {
            ++compressedNum ;
         }
      }
      return len ;
   }

   INT32 _dmsInspect::inspectCappedExtent( CHAR *inBuf, UINT32 inSize,
                                           CHAR *outBuf, UINT32 outSize,
                                           UINT16 collectionID,
                                           dmsCompressorEntry *compressorEntry,
                                           UINT64 &recordNum,
                                           UINT64 &compressedNum,
                                           INT32 &localErr,
                                           pmdEDUCB *cb )
   {
      UINT32 len = 0 ;
      INT32 recordCount = 0 ;
      dmsExtent *extent = (dmsExtent *)inBuf ;
      BOOLEAN isCompressed = FALSE ;

      len += inspectExtentHeader ( inBuf, inSize, outBuf + len,
                                   outSize - len, collectionID, localErr ) ;
      // make sure the extent is valid and in use
      if ( DMS_EXTENT_FLAG_FREED == extent->_flag )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Extent is not in use"OSS_NEWLINE ) ;
         ++localErr ;
      }
      // start inspect all records
      dmsOffset nextRecord = extent->_firstRecordOffset ;
      dmsOffset lastRecord = extent->_lastRecordOffset ;
      while ( DMS_INVALID_OFFSET != nextRecord && len < outSize )
      {
         dmsCappedRecord *record = NULL ;
         INT64 logicalID = -1 ;
         dmsOffset myOffset = DMS_INVALID_OFFSET ;
         if ( nextRecord >= (SINT32)inSize )
         {
            len += ossSnprintf (  outBuf + len, outSize - len,
                                  "Error : nextRecord %d is greater "
                                  "than inSize %d",
                                  nextRecord, inSize ) ;
            ++localErr ;
         }

         record = (dmsCappedRecord*)( ((CHAR*)inBuf) + nextRecord ) ;
         logicalID = record->getLogicalID() ;

         // If we have gone beyond the last record offset, we see it as the end.
         // In that case, no error will be reported.
         if ( logicalID < 0 )
         {
            // If we are still before the last record offset, print the current
            // record, but stop going to the next one.
            if ( nextRecord <= lastRecord )
            {
               len += ossSnprintf( outBuf + len, outSize - len,
                                   "Error: logicalID (%lld) is invalid"OSS_NEWLINE,
                                   logicalID ) ;
               nextRecord = DMS_INVALID_OFFSET ;
            }
            else
            {
               nextRecord = DMS_INVALID_OFFSET ;
               goto exit ;
            }
         }
         else
         {
            // Check if the logical id in the record header matches the position
            // (extent and offset). If not, print the current record, and stop.
            myOffset = logicalID % DMS_CAP_EXTENT_BODY_SZ +
                       DMS_EXTENT_METADATA_SZ ;
            if ( myOffset != nextRecord )
            {
               if ( nextRecord <= lastRecord )
               {
                  len += ossSnprintf( outBuf + len, outSize - len,
                                      "Error: logicalID (%lld) and offset (%u) "
                                      "dose not match"OSS_NEWLINE,
                                      logicalID, nextRecord ) ;
                  nextRecord = DMS_INVALID_OFFSET ;
               }
               else
               {
                  nextRecord = DMS_INVALID_OFFSET ;
                  goto exit ;
               }
            }
         }

         len += inspectCappedDataRecord( cb, record,
                                         outBuf + len, outSize - len,
                                         nextRecord, localErr,
                                         compressorEntry ) ;
         ++recordCount ;
         ++recordNum ;
         if ( isCompressed )
         {
            ++compressedNum ;
         }
         if ( DMS_INVALID_EXTENT != nextRecord )
         {
            nextRecord = ossRoundUpToMultipleX( nextRecord + record->getSize(),
                                                4 ) ;
         }
      }

   exit:
      return len ;
   }

   UINT32 _dmsInspect::inspectDmsLobDataMapBlk( dmsLobDataMapBlk *blk,
                                                CHAR * outBuf, UINT32 outSize,
                                                UINT16 clId,  SINT32 &err )
   {
      UINT32 len           = 0 ;

      if ( blk->_mbID !=  clId )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid mbID, mbId: %c, expected: %d"
                              OSS_NEWLINE, blk->_mbID, clId ) ;
         ++err ;
      }

      if ( blk->_status != DMS_LOB_PAGE_REMOVED &&
           DMS_LOB_PAGE_NORMAL != blk->_status )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid dmsLobDataMapBlk status : "
                              "%c( UNKOWN STATUS )"OSS_NEWLINE,
                              blk->_status ) ;
         ++err ;
      }
      ///TODO:: add bucket list loop inspect.
      return len ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_INSPTDMSLOBMETA, "inspectDmsLobMeta" )
   UINT32 _dmsInspect::inspectDmsLobMeta( dmsLobMeta *lobMeta,
                                          CHAR * outBuf, UINT32 outSize,
                                          SINT32 &err )
   {
      UINT32 len           = 0 ;
      CHAR strTime[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      UINT64 curTime = ossGetCurrentMilliseconds() ;

      if( lobMeta->_flag > DMS_LOB_META_FLAG_PIECESINFO_INSIDE )
      {
         len += ossSnprintf ( outBuf + len , outSize - len,
                              "Error: LobMeta flag  (%d) in lobd file is "
                              "unknown expected less than "
                              "DMS_LOB_META_FLAG_PIECESINFO_INSIDE (%d)"
                              OSS_NEWLINE,
                              lobMeta->_flag,
                              DMS_LOB_META_FLAG_PIECESINFO_INSIDE ) ;
         err++ ;
      }

      if( lobMeta->_version > DMS_LOB_META_CURRENT_VERSION )
      {
         len += ossSnprintf ( outBuf + len , outSize - len,
                              "Error: LobMeta version  (%d) in lobd file is "
                              "unknown expected less than "
                              "DMS_LOB_META_CURRENT_VERSION (%d)"OSS_NEWLINE,
                              lobMeta->_version,
                              DMS_LOB_META_CURRENT_VERSION ) ;
         err++ ;
      }

      if( lobMeta->_status > DMS_LOB_COMPLETE )
      {
         len += ossSnprintf ( outBuf + len , outSize - len,
                              "Error: LobMeta status  (%d) in lobd file is "
                              "unknown expected less than "
                              "DMS_LOB_COMPLETE (%d)"OSS_NEWLINE,
                              lobMeta->_status, DMS_LOB_COMPLETE ) ;
         err++ ;
      }

      if( lobMeta->hasPiecesInfo() && lobMeta->_piecesInfoNum <= 0 )
      {
         len += ossSnprintf ( outBuf + len , outSize - len,
                              "Error: LobMeta piecesInfoNum  (%d)  or flag %d "
                              "(DMS_LOB_META_MERGE_DATA_VERSION) in lobd file "
                              "is unknown"OSS_NEWLINE,
                              lobMeta->_piecesInfoNum, DMS_LOB_COMPLETE) ;
         err++ ;
      }

      ossTimestamp timestamp(lobMeta->_createTime);
      if (curTime < lobMeta->_createTime)
      {
         ossTimestampToString(timestamp, strTime ) ;
         len += ossSnprintf ( outBuf + len , outSize - len,
                              "Error: LobMeta createTime  %lu (%s)  is not "
                              "correct"OSS_NEWLINE,
                              lobMeta->_createTime, strTime ) ;
         err++;
      }

      if ( curTime < lobMeta->_modificationTime )
      {
         timestamp = lobMeta->_modificationTime;
         ossTimestampToString(timestamp, strTime) ;
         len += ossSnprintf( outBuf + len , outSize - len,
                             "Error: LobMeta modificationTime  %lu (%s) "
                             "is not correct"OSS_NEWLINE,
                             lobMeta->_modificationTime, strTime ) ;
         err++;
      }

      return len ;
   }
}


