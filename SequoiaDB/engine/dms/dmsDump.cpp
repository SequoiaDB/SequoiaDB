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

   Source File Name = dmsDump.cpp

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

#include "dmsDump.hpp"
#include "ixm.hpp"
#include "pmdEDU.hpp"
#include "ixmKey.hpp"
#include "dmsCompress.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "utilDictionary.hpp"
#include "dmsStorageDataCapped.hpp"
#include "rtnLobPieces.hpp"

using namespace bson ;

namespace engine
{

   UINT32 _dmsDump::dumpHeader( void *inBuf, UINT32 inSize,
                                CHAR *outBuf, UINT32 outSize,
                                CHAR *addrPrefix, UINT32 options,
                                UINT32 &pageSize, UINT32 &pageNum )
   {
      SDB_ASSERT( outBuf, "outBuf can't be null" ) ;

      UINT32 len                         = 0 ;
      UINT32 hexDumpOption               = 0 ;
      dmsStorageUnitHeader *header       = (dmsStorageUnitHeader*)inBuf ;
      CHAR   eyeCatcher [ DMS_HEADER_EYECATCHER_LEN+1 ] = {0} ;

      if ( NULL == outBuf )
      {
         goto exit ;
      }

      if ( NULL == inBuf || inSize != DMS_HEADER_SZ )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpHeader input size (%d) doesn't match "
                              "expected size (%d)" OSS_NEWLINE,
                              inSize, DMS_HEADER_SZ ) ;
         goto exit ;
      }
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "Storage Unit Header Dump:" OSS_NEWLINE ) ;

      ossMemcpy ( eyeCatcher, header->_eyeCatcher, DMS_HEADER_EYECATCHER_LEN ) ;
      pageSize = header->_pageSize ;
      pageNum  = header->_pageNum ;

      if ( DMS_SU_DMP_OPT_HEX & options )
      {
         if ( DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR & options )
         {
            hexDumpOption |= OSS_HEXDUMP_PREFIX_AS_ADDR ;
         }
         if ( !( DMS_SU_DMP_OPT_HEX_WITH_ASCII & options ) )
         {
            hexDumpOption |= OSS_HEXDUMP_RAW_HEX_ONLY ;
         }
         len += ossHexDumpBuffer( inBuf, inSize, outBuf+len, outSize-len,
                                  addrPrefix, hexDumpOption ) ;
      }
      if ( DMS_SU_DMP_OPT_FORMATTED & options )
      {
         ossTimestamp commitTm ;
         commitTm.time = header->_commitTime / 1000 ;
         commitTm.microtm = ( header->_commitTime % 1000 ) * 1000 ;
         CHAR strTime[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossTimestampToString( commitTm, strTime ) ;

         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Eye Catcher : %s" OSS_NEWLINE,
                              eyeCatcher ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Version     : %d" OSS_NEWLINE,
                              header->_version ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Page Size   : %d" OSS_NEWLINE,
                              header->_pageSize ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Total Size  : %d" OSS_NEWLINE,
                              header->_storageUnitSize ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " SU Name     : %s" OSS_NEWLINE,
                              header->_name ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Sequence    : %d" OSS_NEWLINE,
                              header->_sequence ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Num of Col  : %d" OSS_NEWLINE,
                              header->_numMB ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " HWM of Col  : %d" OSS_NEWLINE,
                              header->_MBHWM ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Page Num    : %d" OSS_NEWLINE,
                              header->_pageNum ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Secret value: 0x%016lx (%llu)" OSS_NEWLINE,
                              header->_secretValue, header->_secretValue ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Lob Page Sz : %d" OSS_NEWLINE,
                              header->_lobdPageSize ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Lob Flag    : %d" OSS_NEWLINE,
                              header->_createLobs ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Commit Flag : %d" OSS_NEWLINE,
                              header->_commitFlag ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Commit LSN  : 0x%016lx (%lld)" OSS_NEWLINE,
                              header->_commitLsn, header->_commitLsn ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Commit Time : %s (%llu)" OSS_NEWLINE,
                              strTime, header->_commitTime ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Unique ID   : 0x%08x (%u)" OSS_NEWLINE,
                              header->_csUniqueID, header->_csUniqueID ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Segment Size: %u" OSS_NEWLINE,
                              header->_segmentSize ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " HWM of IdxInnerID: 0x%08x (%u)" OSS_NEWLINE,
                              header->_idxInnerHWM, header->_idxInnerHWM ) ;

         ossMillisecondsToString( header->_createTime, strTime ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Create Timestamp : %s (%llu)" OSS_NEWLINE,
                              strTime, header->_createTime ) ;

         ossMillisecondsToString( header->_updateTime, strTime ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Update Timestamp : %s (%llu)" OSS_NEWLINE,
                              strTime, header->_updateTime ) ;
      }
      len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;

   exit :
      return len ;
   }

   #define DMS_DUMP_SME_STATE_BUFSZ    63

   UINT32 _dmsDump::dumpSME( void *inBuf, UINT32 inSize,
                             CHAR *outBuf, UINT32 outSize,
                             UINT32 pageNum )
   {
      SDB_ASSERT( outBuf, "outBuf can't be null" ) ;

      UINT32 len            = 0 ;
      UINT32 usedPages      = 0 ;
      UINT32 totalPages     = 0 ;
      CHAR stateBuf [ DMS_DUMP_SME_STATE_BUFSZ + 1 ] = {0} ;
      BOOLEAN hasError      = FALSE ;

      if ( NULL == outBuf )
      {
         goto exit ;
      }

      if ( NULL == inBuf || inSize != DMS_SME_SZ )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpSME input size (%d) doesn't match "
                              "expected size (%d)" OSS_NEWLINE,
                              inSize,
                              DMS_SME_SZ ) ;
         goto exit ;
      }

      len += ossSnprintf ( outBuf + len, outSize - len,
                           "Space Management Extent Dump:" OSS_NEWLINE ) ;

      {
         totalPages  = DMS_MAX_PG ;
         UINT32 beginPage    = 0 ;
         UINT32 endPage      = 0 ;
         dmsSpaceManagementExtent *pSME = ( dmsSpaceManagementExtent* )inBuf ;
         CHAR  currentState = pSME->getBitMask( 0 ) ;
         UINT32 i            = 0 ;

         for ( i = 0 ; i < totalPages ; ++i )
         {
            if ( pSME->getBitMask( i ) != currentState )
            {
               endPage = i - 1 ;
               smeMask2String ( currentState, stateBuf,
                                DMS_DUMP_SME_STATE_BUFSZ ) ;
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    " %010d - %010d [ 0x%02x (%s) ]" OSS_NEWLINE,
                                    beginPage, endPage, currentState,
                                    stateBuf ) ;
               beginPage = i ;
               currentState = pSME->getBitMask( i ) ;
            }

            if ( currentState != DMS_SME_FREE )
            {
               ++usedPages ;
               if ( i >= pageNum )
               {
                  hasError = TRUE ;
               }
            }
         }
         endPage = i - 1 ;
         smeMask2String ( currentState, stateBuf,
                          DMS_DUMP_SME_STATE_BUFSZ ) ;

         len += ossSnprintf ( outBuf + len, outSize - len,
                              " %010d - %010d [ 0x%02x (%s) ]" OSS_NEWLINE,
                              beginPage, endPage, currentState,
                              stateBuf ) ;
      }
      len += ossSnprintf ( outBuf + len, outSize - len,
                           " Total: %d, Allocated: %d, Used: %d" OSS_NEWLINE
                           " Has errors: %s" OSS_NEWLINE,
                           totalPages, pageNum, usedPages,
                           hasError ? "TRUE" : "FALSE" ) ;
      len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;

   exit :
      return len ;
   }

   UINT32 _dmsDump::dumpMME( void *inBuf, UINT32 inSize,
                             CHAR *outBuf, UINT32 outSize,
                             CHAR *addrPrefix, UINT32 options,
                             const CHAR *collectionName,
                             vector< UINT16 > &collections,
                             BOOLEAN force )
   {
      SDB_ASSERT( outBuf, "outBuf can't be null" ) ;

      UINT32 len = 0 ;

      if ( NULL == outBuf )
      {
         goto exit ;
      }

      if ( NULL == inBuf || inSize != DMS_MME_SZ )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpMME input size (%d) doesn't match "
                              "expected size (%d)" OSS_NEWLINE,
                              inSize,
                              DMS_MME_SZ ) ;
         goto exit ;
      }
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "Metadata Management Extent Dump:" OSS_NEWLINE ) ;

      for ( INT32 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         len += dumpMB ( (CHAR*)inBuf + (i*DMS_MB_SIZE), DMS_MB_SIZE,
                         outBuf + len, outSize -len, addrPrefix, options,
                         collectionName, collections, force ) ;
         if ( len == outSize )
         {
            goto exit ;
         }
      }
      len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;

   exit :
      return len ;
   }

   #define DMS_COLLECTION_STATUS_LEN      127

   UINT32 _dmsDump::dumpMB( void *inBuf, UINT32 inSize,
                            CHAR *outBuf, UINT32 outSize,
                            CHAR *addrPrefix, UINT32 options,
                            const CHAR *collectionName,
                            vector< UINT16 > &collections,
                            BOOLEAN force )
   {
      SDB_ASSERT( outBuf, "outBuf can't be null" ) ;

      UINT32 len = 0 ;
      UINT32 hexDumpOption = 0 ;
      dmsMB *mb = (dmsMB*)inBuf ;
      CHAR   tmpStr [ DMS_COLLECTION_STATUS_LEN + 1 ] = {0} ;
      UINT32 tmpInt = 0 , tmpSize = 0 ;
      CHAR uom ;

      if ( NULL == outBuf )
      {
         goto exit ;
      }

      if ( NULL == inBuf || inSize != DMS_MB_SIZE )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpMMEMetadataBlock input size (%d) "
                              "doesn't match expected size (%d)" OSS_NEWLINE,
                              inSize, DMS_MB_SIZE ) ;
         goto exit ;
      }

      // if we want to find a specific collection
      if ( collectionName )
      {
         if ( ossStrncmp ( mb->_collectionName, collectionName,
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
      }
      else if ( !force )
      {
         // if not enable force, when mb is invalid, ignored
         goto exit ;
      }

      if ( DMS_SU_DMP_OPT_HEX & options )
      {
         if ( DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR & options )
         {
            hexDumpOption |= OSS_HEXDUMP_PREFIX_AS_ADDR ;
         }
         if ( !( DMS_SU_DMP_OPT_HEX_WITH_ASCII & options ) )
         {
            hexDumpOption |= OSS_HEXDUMP_RAW_HEX_ONLY ;
         }
         len += ossHexDumpBuffer( inBuf, inSize, outBuf+len, outSize-len,
                                  addrPrefix, hexDumpOption ) ;
      }

      if ( DMS_SU_DMP_OPT_FORMATTED & options )
      {
         mbFlag2String ( mb->_flag, tmpStr, DMS_COLLECTION_STATUS_LEN ) ;

         len += ossSnprintf( outBuf + len, outSize - len,
                             OSS_NEWLINE" Collection name   : %s" OSS_NEWLINE,
                             mb->_collectionName ) ;

         len += ossSnprintf( outBuf + len, outSize - len,
                             " CL Unique ID      : %ld" OSS_NEWLINE,
                             mb->_clUniqueID ) ;

         len += ossSnprintf( outBuf + len, outSize - len,
                             " Flag              : 0x%04lx (%s)" OSS_NEWLINE,
                             mb->_flag, tmpStr ) ;

         mbAttr2String ( mb->_attributes, tmpStr, DMS_COLLECTION_STATUS_LEN ) ;
         len += ossSnprintf( outBuf + len, outSize - len,
                             " Attributes        : 0x%04lx (%s)" OSS_NEWLINE,
                             mb->_attributes, tmpStr ) ;

         len += ossSnprintf( outBuf + len, outSize - len,
                             " Collection ID     : %u" OSS_NEWLINE,
                              mb->_blockID ) ;

         const CHAR *compressorType = NULL ;
         if ( UTIL_COMPRESSOR_LZW == mb->_compressorType )
         {
            compressorType = "lzw" ;
         }
         else if ( UTIL_COMPRESSOR_SNAPPY == mb->_compressorType )
         {
            compressorType = "snappy" ;
         }
         else if ( UTIL_COMPRESSOR_LZ4 == mb->_compressorType )
         {
            compressorType = "lz4" ;
         }
         else if ( UTIL_COMPRESSOR_ZLIB == mb->_compressorType )
         {
            compressorType = "zlib" ;
         }
         else if ( UTIL_COMPRESSOR_INVALID == mb->_compressorType )
         {
            compressorType = "none" ;
         }
         else
         {
            compressorType = "invalid type" ;
         }

         len += ossSnprintf( outBuf + len, outSize - len,
                             " First extent ID   : 0x%08x (%d)" OSS_NEWLINE
                             " Last extent ID    : 0x%08x (%d)" OSS_NEWLINE
                             " Logical ID        : 0x%08x (%d)" OSS_NEWLINE
                             " Index HWM         : %u" OSS_NEWLINE
                             " Number of indexes : %u" OSS_NEWLINE
                             " First Load ExtID  : 0x%08x (%d)" OSS_NEWLINE
                             " Last Load ExtID   : 0x%08x (%d)" OSS_NEWLINE
                             " Expand extent ID  : 0x%08x (%d)" OSS_NEWLINE,
                             mb->_firstExtentID, mb->_firstExtentID,
                             mb->_lastExtentID, mb->_lastExtentID,
                             mb->_logicalID, mb->_logicalID,
                             mb->_indexHWCount,
                             mb->_numIndexes,
                             mb->_loadFirstExtentID, mb->_loadFirstExtentID,
                             mb->_loadLastExtentID, mb->_loadLastExtentID,
                             mb->_mbExExtentID, mb->_mbExExtentID ) ;

         /// stat
         len += ossSnprintf( outBuf + len, outSize - len,
                             " Total records     : %llu" OSS_NEWLINE
                             " Total lobs        : %llu" OSS_NEWLINE
                             " Total data pages  : %u" OSS_NEWLINE
                             " Total data free sp: %llu" OSS_NEWLINE
                             " Total index pages : %u" OSS_NEWLINE
                             " Total idx free sp : %llu" OSS_NEWLINE
                             " Total lob pages   : %u" OSS_NEWLINE
                             " Total org data len: %llu" OSS_NEWLINE
                             " total data len    : %llu" OSS_NEWLINE,
                             mb->_totalRecords,
                             mb->_totalLobs,
                             mb->_totalDataPages,
                             mb->_totalDataFreeSpace,
                             mb->_totalIndexPages,
                             mb->_totalIndexFreeSpace,
                             mb->_totalLobPages,
                             mb->_totalOrgDataLen,
                             mb->_totalDataLen ) ;

         /// compress
         len += ossSnprintf( outBuf + len, outSize - len,
                             " Dict extent ID    : 0x%08x (%d)" OSS_NEWLINE
                             " New Dict extent ID: 0x%08x (%d)" OSS_NEWLINE
                             " Dict stat page ID : 0x%08x (%d)" OSS_NEWLINE
                             " Dictionary version: %u" OSS_NEWLINE
                             " Compression Type  : 0x%02x (%s)" OSS_NEWLINE
                             " Last comp ratio   : %d" OSS_NEWLINE,
                             mb->_dictExtentID, mb->_dictExtentID,
                             mb->_newDictExtentID, mb->_newDictExtentID,
                             mb->_dictStatPageID, mb->_dictStatPageID,
                             mb->_dictVersion,
                             mb->_compressorType, compressorType,
                             mb->_lastCompressRatio ) ;

         ossTimestamp dataTm ;
         dataTm.time = mb->_commitTime / 1000 ;
         dataTm.microtm = ( mb->_commitTime % 1000 ) * 1000 ;
         CHAR strDataTime[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossTimestampToString( dataTm, strDataTime ) ;

         ossTimestamp idxTm ;
         idxTm.time = mb->_idxCommitTime / 1000 ;
         idxTm.microtm = ( mb->_idxCommitTime % 1000 ) * 1000 ;
         CHAR strIdxTime[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossTimestampToString( idxTm, strIdxTime ) ;

         ossTimestamp lobTm ;
         lobTm.time = mb->_lobCommitTime / 1000 ;
         lobTm.microtm = ( mb->_lobCommitTime % 1000 ) * 1000 ;
         CHAR strLobTime[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossTimestampToString( lobTm, strLobTime ) ;

         /// commit info
         len += ossSnprintf( outBuf + len, outSize - len,
                             " Data Commit Flag  : %d" OSS_NEWLINE
                             " Data Commit LSN   : 0x%016lx (%lld)" OSS_NEWLINE
                             " Data Commit Time  : %s (%llu)" OSS_NEWLINE
                             " Idx Commit Flag   : %d" OSS_NEWLINE
                             " Idx Commit LSN    : 0x%016lx (%lld)" OSS_NEWLINE
                             " Idx Commit Time   : %s (%llu)" OSS_NEWLINE
                             " Lob Commit Flag   : %d" OSS_NEWLINE
                             " Lob Commit LSN    : 0x%016lx (%lld)" OSS_NEWLINE
                             " Lob Commit Time   : %s (%llu)" OSS_NEWLINE,
                             mb->_commitFlag,
                             mb->_commitLSN, mb->_commitLSN,
                             strDataTime, mb->_commitTime,
                             mb->_idxCommitFlag,
                             mb->_idxCommitLSN, mb->_idxCommitLSN,
                             strIdxTime, mb->_idxCommitTime,
                             mb->_lobCommitFlag,
                             mb->_lobCommitLSN, mb->_lobCommitLSN,
                             strLobTime, mb->_lobCommitTime ) ;
         // Extent option extent id
         len += ossSnprintf( outBuf + len, outSize - len,
                             " Extend option extent ID: 0x%08x (%d)" OSS_NEWLINE,
                             mb->_mbOptExtentID, mb->_mbOptExtentID ) ;

         // create timestamp
         CHAR strCreateTime[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossMillisecondsToString( mb->_createTime, strCreateTime ) ;

         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Create Timestamp  : %s (%llu)" OSS_NEWLINE,
                              strCreateTime, mb->_createTime ) ;

         // update timestamp
         CHAR strUpdateTime[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossMillisecondsToString( mb->_updateTime, strUpdateTime ) ;

         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Update Timestamp  : %s (%llu)" OSS_NEWLINE,
                              strUpdateTime, mb->_updateTime ) ;

         // Delete list
         len += ossSnprintf( outBuf + len, outSize - len,
                             " Deleted list :" OSS_NEWLINE ) ;
         tmpInt = 16 ;
         for ( UINT16 i = 0 ; i < dmsMB::_max ; i++ )
         {
            tmpInt <<= 1 ;
            if ( tmpInt < 1024 )
            {
               tmpSize = tmpInt & 0x3FF ; // tmpInt % 1024
               uom = ' ' ;
            }
            else if ( tmpInt < 1048576 )
            {
               tmpSize = tmpInt >> 10 ;  // tmpInt / 1024
               uom = 'K' ;
            }
            else
            {
               tmpSize = tmpInt >> 20 ;  // tmpInt / 1048576
               uom = 'M' ;
            }
            if ( !force &&
                 DMS_INVALID_EXTENT == mb->_deleteList[i]._extent &&
                 DMS_INVALID_OFFSET == mb->_deleteList[i]._offset )
            {
               continue ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                "   %3u%c : %08x %08x" OSS_NEWLINE,
                                tmpSize, uom,
                                mb->_deleteList[i]._extent,
                                mb->_deleteList[i]._offset ) ;
         }

         // index list
         len += ossSnprintf( outBuf + len, outSize - len,
                             " Index extent:" OSS_NEWLINE ) ;

         for ( UINT16 i = 0 ; i < DMS_COLLECTION_MAX_INDEX ; i++ )
         {
            if ( !force && DMS_INVALID_EXTENT == mb->_indexExtent[i] )
            {
               continue ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                "   %2u : 0x%08x" OSS_NEWLINE,
                                i, mb->_indexExtent[i] ) ;
         }
      }

   exit :
      return len ;
   }

   UINT32 _dmsDump::dumpRawPage( void *inBuf, UINT32 inSize,
                                 CHAR *outBuf, UINT32 outSize )
   {
      UINT32 len = 0 ;
      len += ossHexDumpBuffer( inBuf, inSize, outBuf+len, outSize-len,
                               NULL, OSS_HEXDUMP_PREFIX_AS_ADDR ) ;
      return len ;
   }

   UINT32 _dmsDump::dumpExtentHeader( void *inBuf, UINT32 inSize,
                                      CHAR *outBuf, UINT32 outSize )
   {
      SDB_ASSERT( outBuf, "outBuf can't be null" ) ;

      UINT32 len           = 0 ;

      if ( NULL == outBuf )
      {
         goto exit ;
      }

      if ( NULL == inBuf || inSize < sizeof(dmsExtent) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpExtentHeader input size (%d) "
                              "is too small" OSS_NEWLINE,
                              inSize ) ;
         goto exit ;
      }

   exit :
      return len ;
   }

   UINT32 _dmsDump::_dumpExtentHeaderComm( const dmsExtent *extent,
                                           CHAR *outBuf, UINT32 outSize )
   {
      SDB_ASSERT( extent, "extent can't be null" ) ;
      SDB_ASSERT( outBuf, "outBuf can't be null" ) ;

      UINT32 len = 0 ;

      if ( NULL == outBuf || NULL == extent )
      {
         goto exit ;
      }

      len += ossSnprintf ( outBuf, outSize,
                           "    Eye Catcher  : %c%c" OSS_NEWLINE,
                           extent->_eyeCatcher[0], extent->_eyeCatcher[1] ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Extent Size  : %u" OSS_NEWLINE,
                           extent->_blockSize ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    CollectionID : %u" OSS_NEWLINE,
                           extent->_mbID ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Flag         : 0x%02x (%s)" OSS_NEWLINE,
                           extent->_flag, extent->_flag==DMS_EXTENT_FLAG_INUSE ?
                           "InUse" : "Free" ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Version      : %d" OSS_NEWLINE,
                           extent->_version ) ;
   exit:
      return len ;
   }

   #define DMS_DUMP_IXM_CB_FLAG_TEXT_LEN        63

   UINT32 _dmsDump::dumpIndexCBExtentHeader( void *inBuf, UINT32 inSize,
                                             CHAR *outBuf, UINT32 outSize )
   {
      SDB_ASSERT( outBuf, "outBuf can't be null" ) ;

      UINT32 len           = 0 ;
      ixmIndexCBExtent *header = (ixmIndexCBExtent*)inBuf ;
      CHAR tmpBuff [ DMS_DUMP_IXM_CB_FLAG_TEXT_LEN + 1 ] = {0} ;

      if ( NULL == outBuf )
      {
         goto exit ;
      }

      if ( NULL == inBuf ||
           inSize < sizeof(ixmIndexCBExtent) || inSize % DMS_PAGE_SIZE4K != 0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpIndexCBExtentHeader input size (%d) "
                              "is too small or not aligned with 4K" OSS_NEWLINE,
                              inSize ) ;
         goto exit ;
      }

      if ( header->_eyeCatcher[0] != IXM_EXTENT_CB_EYECATCHER0 ||
           header->_eyeCatcher[1] != IXM_EXTENT_CB_EYECATCHER1 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid eye catcher: %c%c" OSS_NEWLINE,
                              header->_eyeCatcher[0], header->_eyeCatcher[1] ) ;
         goto exit ;
      }

      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Eye Catcher  : %c%c" OSS_NEWLINE,
                           header->_eyeCatcher[0], header->_eyeCatcher[1] ) ;

      ossStrncpy ( tmpBuff, ixmGetIndexFlagDesp(header->_indexFlag),
                   DMS_DUMP_IXM_CB_FLAG_TEXT_LEN ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Index Flags  : %d (%s)" OSS_NEWLINE,
                           header->_indexFlag, tmpBuff ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    CollectionID : %u" OSS_NEWLINE,
                           header->_mbID ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Flag         : 0x%02x (%s)" OSS_NEWLINE,
                           header->_flag, header->_flag==DMS_EXTENT_FLAG_INUSE ?
                           "InUse" : "Free" ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Version      : %d" OSS_NEWLINE,
                           header->_version ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "  Logical ID     : %d" OSS_NEWLINE,
                           header->_logicID ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Root Ext     : 0x%08x (%d)" OSS_NEWLINE,
                           header->_rootExtentID, header->_rootExtentID ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "Scan extent LID  : 0x%08x (%d)" OSS_NEWLINE,
                           header->_scanExtLID, header->_scanExtLID ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "Scan extent Offset  : 0x%08x (%d)" OSS_NEWLINE,
                           header->_scanExtOffset, header->_scanExtOffset ) ;
      ossStrncpy ( tmpBuff, ixmGetIndexTypeDesp(header->_type).c_str(),
                   DMS_DUMP_IXM_CB_FLAG_TEXT_LEN ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "     Type        : %d (%s)" OSS_NEWLINE,
                           header->_type, tmpBuff ) ;
      try
      {
         BSONObj indexDef ( ((CHAR*)inBuf+sizeof(ixmIndexCBExtent)) ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "    Index Def    : %s" OSS_NEWLINE,
                              indexDef.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Failed to extract index def: %s"
                              OSS_NEWLINE,
                              e.what() ) ;
      }

   exit :
      return len ;
   }

   UINT32 _dmsDump::dumpIndexCBExtent( void *inBuf, UINT32 inSize,
                                       CHAR *outBuf, UINT32 outSize,
                                       CHAR *addrPrefix, UINT32 options,
                                       dmsExtentID &root )
   {
      SDB_ASSERT( outBuf, "outBuf can't be null" ) ;

      UINT32 len           = 0 ;
      UINT32 hexDumpOption = 0 ;
      ixmIndexCBExtent *extent = (ixmIndexCBExtent*)inBuf ;

      if ( NULL == outBuf )
      {
         goto exit ;
      }

      if ( NULL == inBuf ||
           inSize < sizeof(ixmIndexCBExtent) || inSize % DMS_PAGE_SIZE4K != 0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpIndexCBExtent input size (%d) "
                              "is too small or not aligned with 4K" OSS_NEWLINE,
                              inSize ) ;
         goto exit ;
      }

      if ( extent->_eyeCatcher[0] != IXM_EXTENT_CB_EYECATCHER0 ||
           extent->_eyeCatcher[1] != IXM_EXTENT_CB_EYECATCHER1 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid eye catcher: %c%c" OSS_NEWLINE,
                              extent->_eyeCatcher[0], extent->_eyeCatcher[1] ) ;
         goto exit ;
      }

      if ( DMS_SU_DMP_OPT_HEX & options )
      {
         if ( DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR & options )
         {
            hexDumpOption |= OSS_HEXDUMP_PREFIX_AS_ADDR ;
         }
         if ( !( DMS_SU_DMP_OPT_HEX_WITH_ASCII & options ) )
         {
            hexDumpOption |= OSS_HEXDUMP_RAW_HEX_ONLY ;
         }
         len += ossHexDumpBuffer( inBuf, inSize, outBuf+len, outSize-len,
                                  addrPrefix, hexDumpOption ) ;
      }

      root = extent->_rootExtentID ;

      if ( DMS_SU_DMP_OPT_FORMATTED & options )
      {
         len += dumpIndexCBExtentHeader ( inBuf, inSize,
                                          outBuf+len, outSize-len ) ;
      }

   exit :
      return len ;
   }

}


