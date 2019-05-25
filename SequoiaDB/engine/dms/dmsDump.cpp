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
#include "ixmExtent.hpp"
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

   static void appendString( CHAR * pBuffer, INT32 bufSize,
                             const CHAR *flagStr )
   {
      if ( 0 != *pBuffer )
      {
         ossStrncat( pBuffer, " | ", bufSize - ossStrlen( pBuffer ) ) ;
      }
      ossStrncat( pBuffer, flagStr, bufSize - ossStrlen( pBuffer ) ) ;
   }

   const CHAR* getIndexFlagDesp( UINT16 indexFlag )
   {
      switch ( indexFlag )
      {
      case IXM_INDEX_FLAG_NORMAL :
         return "Normal" ;
         break ;
      case IXM_INDEX_FLAG_CREATING :
         return "Creating" ;
         break ;
      case IXM_INDEX_FLAG_DROPPING :
         return "Dropping" ;
         break ;
      case IXM_INDEX_FLAG_INVALID :
         return "Invalid" ;
         break ;
      case IXM_INDEX_FLAG_TRUNCATING :
         return "Truncating" ;
         break ;
      default :
         break ;
      }
      return "Unknow" ;
   }

   #define DMS_INDEXTYPE_TMP_STR_SZ       63

   string getIndexTypeDesp( UINT16 type )
   {
      CHAR szTmp[DMS_INDEXTYPE_TMP_STR_SZ+1] = {0} ;
      if ( IXM_EXTENT_TYPE_NONE == type )
      {
         return "None" ;
      }

      if ( IXM_EXTENT_HAS_TYPE( type, IXM_EXTENT_TYPE_POSITIVE ) )
      {
         ossStrncat( szTmp, "Positive", DMS_INDEXTYPE_TMP_STR_SZ ) ;
         OSS_BIT_CLEAR( type, IXM_EXTENT_TYPE_POSITIVE ) ;
      }
      if ( IXM_EXTENT_HAS_TYPE( type, IXM_EXTENT_TYPE_REVERSE ) )
      {
         appendString( szTmp, DMS_INDEXTYPE_TMP_STR_SZ, "Reverse" ) ;
         OSS_BIT_CLEAR( type, IXM_EXTENT_TYPE_REVERSE ) ;
      }
      if ( IXM_EXTENT_HAS_TYPE( type, IXM_EXTENT_TYPE_2D ) )
      {
         appendString( szTmp, DMS_INDEXTYPE_TMP_STR_SZ, "2d" ) ;
         OSS_BIT_CLEAR( type, IXM_EXTENT_TYPE_2D ) ;
      }
      if ( IXM_EXTENT_HAS_TYPE( type, IXM_EXTENT_TYPE_TEXT ) )
      {
         appendString( szTmp, DMS_INDEXTYPE_TMP_STR_SZ, "Text" ) ;
         OSS_BIT_CLEAR( type, IXM_EXTENT_TYPE_TEXT ) ;
      }

      if ( type )
      {
         appendString( szTmp, DMS_INDEXTYPE_TMP_STR_SZ, "Unknow" ) ;
      }

      return szTmp ;
   }

   UINT32 _dmsDump::dumpHeader( void *inBuf, UINT32 inSize,
                                CHAR *outBuf, UINT32 outSize,
                                CHAR *addrPrefix, UINT32 options,
                                UINT32 &pageSize, UINT32 &pageNum )
   {
      UINT32 len                         = 0 ;
      UINT32 hexDumpOption               = 0 ;
      dmsStorageUnitHeader *header       = (dmsStorageUnitHeader*)inBuf ;
      CHAR   eyeCatcher [ DMS_HEADER_EYECATCHER_LEN+1 ] = {0} ;

      if ( NULL == inBuf || NULL == outBuf || inSize != DMS_HEADER_SZ )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpHeader input size (%d) doesn't match "
                              "expected size (%d)"OSS_NEWLINE,
                              inSize, DMS_HEADER_SZ ) ;
         goto exit ;
      }
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "Storage Unit Header Dump:"OSS_NEWLINE ) ;

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
                              " Eye Catcher : %s"OSS_NEWLINE,
                              eyeCatcher ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Version     : %d"OSS_NEWLINE,
                              header->_version ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Page Size   : %d"OSS_NEWLINE,
                              header->_pageSize ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Total Size  : %d"OSS_NEWLINE,
                              header->_storageUnitSize ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " SU Name     : %s"OSS_NEWLINE,
                              header->_name ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Sequence    : %d"OSS_NEWLINE,
                              header->_sequence ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Num of Col  : %d"OSS_NEWLINE,
                              header->_numMB ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " HWM of Col  : %d"OSS_NEWLINE,
                              header->_MBHWM ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Page Num    : %d"OSS_NEWLINE,
                              header->_pageNum ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Secret value: 0x%016lx (%llu)"OSS_NEWLINE,
                              header->_secretValue, header->_secretValue ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Lob Page Sz : %d"OSS_NEWLINE,
                              header->_lobdPageSize ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Lob Flag    : %d"OSS_NEWLINE,
                              header->_createLobs ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Commit Flag : %d"OSS_NEWLINE,
                              header->_commitFlag ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Commit LSN  : 0x%016lx (%lld)"OSS_NEWLINE,
                              header->_commitLsn, header->_commitLsn ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " Commit Time : %s (%llu)"OSS_NEWLINE,
                              strTime, header->_commitTime ) ;
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
      UINT32 len            = 0 ;
      UINT32 usedPages      = 0 ;
      UINT32 totalPages     = 0 ;
      CHAR stateBuf [ DMS_DUMP_SME_STATE_BUFSZ + 1 ] = {0} ;
      BOOLEAN hasError      = FALSE ;

      if ( NULL == inBuf || NULL == outBuf || inSize != DMS_SME_SZ )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpSME input size (%d) doesn't match "
                              "expected size (%d)"OSS_NEWLINE,
                              inSize,
                              DMS_SME_SZ ) ;
         goto exit ;
      }

      len += ossSnprintf ( outBuf + len, outSize - len,
                           "Space Management Extent Dump:"OSS_NEWLINE ) ;

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
                                    " %010d - %010d [ 0x%02x (%s) ]"OSS_NEWLINE,
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
                              " %010d - %010d [ 0x%02x (%s) ]"OSS_NEWLINE,
                              beginPage, endPage, currentState,
                              stateBuf ) ;
      }
      len += ossSnprintf ( outBuf + len, outSize - len,
                           " Total: %d, Allocated: %d, Used: %d"OSS_NEWLINE
                           " Has errors: %s"OSS_NEWLINE,
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
      UINT32 len = 0 ;

      if ( NULL == inBuf || NULL == outBuf || inSize != DMS_MME_SZ )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpMME input size (%d) doesn't match "
                              "expected size (%d)"OSS_NEWLINE,
                              inSize,
                              DMS_MME_SZ ) ;
         goto exit ;
      }
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "Metadata Management Extent Dump:"OSS_NEWLINE ) ;

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
      UINT32 len = 0 ;
      UINT32 hexDumpOption = 0 ;
      dmsMB *mb = (dmsMB*)inBuf ;
      CHAR   tmpStr [ DMS_COLLECTION_STATUS_LEN + 1 ] = {0} ;
      UINT32 tmpInt = 0 , tmpSize = 0 ;
      CHAR uom ;

      if ( NULL == inBuf || NULL == outBuf || inSize != DMS_MB_SIZE )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpMMEMetadataBlock input size (%d) "
                              "doesn't match expected size (%d)"OSS_NEWLINE,
                              inSize, DMS_MB_SIZE ) ;
         goto exit ;
      }

      if ( collectionName )
      {
         if ( ossStrncmp ( mb->_collectionName, collectionName,
                           DMS_COLLECTION_NAME_SZ ) != 0 )
         {
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
                             OSS_NEWLINE" Collection name   : %s"OSS_NEWLINE,
                             mb->_collectionName ) ;
         len += ossSnprintf( outBuf + len, outSize - len,
                             " Flag              : 0x%04lx (%s)"OSS_NEWLINE,
                             mb->_flag, tmpStr ) ;

         mbAttr2String ( mb->_attributes, tmpStr, DMS_COLLECTION_STATUS_LEN ) ;
         len += ossSnprintf( outBuf + len, outSize - len,
                             " Attributes        : 0x%04lx (%s)"OSS_NEWLINE,
                             mb->_attributes, tmpStr ) ;

         len += ossSnprintf( outBuf + len, outSize - len,
                             " Collection ID     : %u"OSS_NEWLINE,
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
         else if ( UTIL_COMPRESSOR_INVALID == mb->_compressorType )
         {
            compressorType = "none" ;
         }
         else
         {
            compressorType = "invalid type" ;
         }

         len += ossSnprintf( outBuf + len, outSize - len,
                             " First extent ID   : 0x%08x (%d)"OSS_NEWLINE
                             " Last extent ID    : 0x%08x (%d)"OSS_NEWLINE
                             " Logical ID        : 0x%08x (%d)"OSS_NEWLINE
                             " Index HWM         : %u"OSS_NEWLINE
                             " Number of indexes : %u"OSS_NEWLINE
                             " First Load ExtID  : 0x%08x (%d)"OSS_NEWLINE
                             " Last Load ExtID   : 0x%08x (%d)"OSS_NEWLINE
                             " Expand extent ID  : 0x%08x (%d)"OSS_NEWLINE,
                             mb->_firstExtentID, mb->_firstExtentID,
                             mb->_lastExtentID, mb->_lastExtentID,
                             mb->_logicalID, mb->_logicalID,
                             mb->_indexHWCount,
                             mb->_numIndexes,
                             mb->_loadFirstExtentID, mb->_loadFirstExtentID,
                             mb->_loadLastExtentID, mb->_loadLastExtentID,
                             mb->_mbExExtentID, mb->_mbExExtentID ) ;

         len += ossSnprintf( outBuf + len, outSize - len,
                             " Total records     : %llu"OSS_NEWLINE
                             " Total lobs        : %llu"OSS_NEWLINE
                             " Total data pages  : %u"OSS_NEWLINE
                             " Total data free sp: %llu"OSS_NEWLINE
                             " Total index pages : %u"OSS_NEWLINE
                             " Total idx free sp : %llu"OSS_NEWLINE
                             " Total lob pages   : %u"OSS_NEWLINE
                             " Total org data len: %llu"OSS_NEWLINE
                             " total data len    : %llu"OSS_NEWLINE,
                             mb->_totalRecords,
                             mb->_totalLobs,
                             mb->_totalDataPages,
                             mb->_totalDataFreeSpace,
                             mb->_totalIndexPages,
                             mb->_totalIndexFreeSpace,
                             mb->_totalLobPages,
                             mb->_totalOrgDataLen,
                             mb->_totalDataLen ) ;

         len += ossSnprintf( outBuf + len, outSize - len,
                             " Dict extent ID    : 0x%08x (%d)"OSS_NEWLINE
                             " New Dict extent ID: 0x%08x (%d)"OSS_NEWLINE
                             " Dict stat page ID : 0x%08x (%d)"OSS_NEWLINE
                             " Dictionary version: %u"OSS_NEWLINE
                             " Compression Type  : 0x%02x (%s)"OSS_NEWLINE
                             " Last comp ratio   : %d"OSS_NEWLINE,
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

         len += ossSnprintf( outBuf + len, outSize - len,
                             " Data Commit Flag  : %d"OSS_NEWLINE
                             " Data Commit LSN   : 0x%016lx (%lld)"OSS_NEWLINE
                             " Data Commit Time  : %s (%llu)"OSS_NEWLINE
                             " Idx Commit Flag   : %d"OSS_NEWLINE
                             " Idx Commit LSN    : 0x%016lx (%lld)"OSS_NEWLINE
                             " Idx Commit Time   : %s (%llu)"OSS_NEWLINE
                             " Lob Commit Flag   : %d"OSS_NEWLINE
                             " Lob Commit LSN    : 0x%016lx (%lld)"OSS_NEWLINE
                             " Lob Commit Time   : %s (%llu)"OSS_NEWLINE,
                             mb->_commitFlag,
                             mb->_commitLSN, mb->_commitLSN,
                             strDataTime, mb->_commitTime,
                             mb->_idxCommitFlag,
                             mb->_idxCommitLSN, mb->_idxCommitLSN,
                             strIdxTime, mb->_idxCommitTime,
                             mb->_lobCommitFlag,
                             mb->_lobCommitLSN, mb->_lobCommitLSN,
                             strLobTime, mb->_lobCommitTime ) ;
         len += ossSnprintf( outBuf + len, outSize - len,
                             " Extend option extent ID: 0x%08x (%d)"OSS_NEWLINE,
                             mb->_mbOptExtentID, mb->_mbOptExtentID ) ;
         len += ossSnprintf( outBuf + len, outSize - len,
                             " Deleted list :"OSS_NEWLINE ) ;
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
                                "   %3u%c : %08x %08x"OSS_NEWLINE,
                                tmpSize, uom,
                                mb->_deleteList[i]._extent,
                                mb->_deleteList[i]._offset ) ;
         }

         len += ossSnprintf( outBuf + len, outSize - len,
                             " Index extent:"OSS_NEWLINE ) ;

         for ( UINT16 i = 0 ; i < DMS_COLLECTION_MAX_INDEX ; i++ )
         {
            if ( !force && DMS_INVALID_EXTENT == mb->_indexExtent[i] )
            {
               continue ;
            }
            len += ossSnprintf( outBuf + len, outSize - len,
                                "   %2u : 0x%08x"OSS_NEWLINE,
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

   UINT32 _dmsDump::dumpMBEx( void * inBuf, UINT32 inSize, CHAR * outBuf,
                              UINT32 outSize, CHAR * addrPrefix,
                              UINT32 options, dmsExtentID extID )
   {
      UINT32 len           = 0 ;
      UINT32 hexDumpOption = 0 ;
      dmsMBEx *mbEx        = ( dmsMBEx* )inBuf ;

      if ( NULL == inBuf || NULL == outBuf || inSize < sizeof( dmsMBEx ) ||
           inSize % DMS_PAGE_SIZE4K != 0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpMBEx input size (%d) "
                              "is too small or not aligned with 4K"OSS_NEWLINE,
                              inSize ) ;
         goto exit ;
      }

      if ( mbEx->_header._eyeCatcher[0] != DMS_META_EXTENT_EYECATCHER0 ||
           mbEx->_header._eyeCatcher[1] != DMS_META_EXTENT_EYECATCHER1 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
                              mbEx->_header._eyeCatcher[0],
                              mbEx->_header._eyeCatcher[1] ) ;
         goto exit ;
      }

      len += ossSnprintf ( outBuf + len, outSize - len,
                           " ExtentID: 0x%08x (%d)"OSS_NEWLINE,
                           extID, extID ) ;

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
         dmsExtentID firstID = DMS_INVALID_EXTENT ;
         dmsExtentID lastID  = DMS_INVALID_EXTENT ;
         UINT32 usedSegNum   = 0 ;
         BOOLEAN hasError    = FALSE ;

         len += ossSnprintf ( outBuf + len, outSize -len,
                              " Meta Extent Header :"OSS_NEWLINE ) ;
         len += dumpExtentHeader ( inBuf, inSize, outBuf + len,
                                   outSize - len ) ;
         if ( DMS_EXTENT_FLAG_FREED == mbEx->_header._flag )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Extent is not in use"OSS_NEWLINE ) ;
            goto exit ;
         }

         len += ossSnprintf ( outBuf + len, outSize -len,
                              " Segment extent info :"OSS_NEWLINE ) ;
         for ( UINT32 i = 0 ; i < mbEx->_header._segNum ; ++i )
         {
            mbEx->getFirstExtentID( i, firstID ) ;
            mbEx->getLastExtentID( i, lastID ) ;

            if ( DMS_INVALID_EXTENT != firstID ||
                 DMS_INVALID_EXTENT != lastID )
            {
               ++usedSegNum ;
               if ( DMS_INVALID_EXTENT == firstID ||
                    DMS_INVALID_EXTENT == lastID )
               {
                  hasError = TRUE ;
               }

               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "  %6u : [0x%08x, 0x%08x]"OSS_NEWLINE,
                                    i, firstID, lastID ) ;
            }
         } // end for
         len += ossSnprintf ( outBuf + len, outSize -len,
                              " Used segment num: %d, has error: %s"OSS_NEWLINE,
                              usedSegNum, hasError ? "TRUE" : "FALSE" ) ;
      }
      len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;

   exit:
      return len ;
   }

   UINT32 _dmsDump::_dumpDictDetail( void *inBuf, UINT32 inSize,
                                     CHAR *outBuf, UINT32 outSize )
   {
      UINT32 len = 0 ;
      utilDictionaryDetail detail ;

      utilDictHead *head
         = (utilDictHead *)( (CHAR*)inBuf + sizeof( dmsDictExtent ) ) ;
      if ( UTIL_DICT_LZW == head->_type )
      {
         getDictionaryDetail( (void *)head, detail ) ;
         len += ossSnprintf( outBuf + len, outSize - len,
                             "Dictionary detail:"OSS_NEWLINE ) ;
         len += ossSnprintf( outBuf + len, outSize - len,
                             "   Type: %s"OSS_NEWLINE,
                             VALUE_NAME_LZW ) ;
         len += ossSnprintf( outBuf + len, outSize - len,
                             "   Version: %u"OSS_NEWLINE,
                             detail._version ) ;
         len += ossSnprintf( outBuf + len, outSize - len,
                             "   Maximum code: %u"OSS_NEWLINE,
                             detail._maxCode ) ;
         len += ossSnprintf( outBuf + len, outSize - len,
                             "   Code size: %u"OSS_NEWLINE,
                             detail._codeSize ) ;
      }

      return len ;
   }

   UINT32 _dmsDump::_dumpExtOptionDetail( CHAR *inBuf, UINT32 inSize,
                                          CHAR *outBuf, UINT32 outSize,
                                          DMS_STORAGE_TYPE type )
   {
      UINT32 len = 0 ;
      if ( DMS_STORAGE_CAPPED == type )
      {
         dmsCappedCLOptions *options =
            (dmsCappedCLOptions *)(inBuf + DMS_OPTEXTENT_HEADER_SZ) ;
         len += ossSnprintf( outBuf + len, outSize - len,
                             "Extend option detail:"OSS_NEWLINE ) ;
         len += ossSnprintf( outBuf + len, outSize - len,
                             "   Size: %lld"OSS_NEWLINE, options->_maxSize ) ;
         len += ossSnprintf( outBuf + len, outSize - len,
                             "   Max: %lld"OSS_NEWLINE, options->_maxRecNum ) ;
         len += ossSnprintf( outBuf + len, outSize - len,
                             "   Overwrite: %s"OSS_NEWLINE,
                             (options->_overwrite) ? "true" : "false" ) ;
      }
      return len ;
   }

   UINT32 _dmsDump::dumpDictExtent( void * inBuf, UINT32 inSize, CHAR * outBuf,
                                    UINT32 outSize, CHAR * addrPrefix,
                                    UINT32 options, dmsExtentID extID )
   {
      UINT32 len = 0 ;
      UINT32 hexDumpOption = 0 ;
      dmsDictExtent *extent = (dmsDictExtent*)inBuf ;

      if ( NULL == inBuf || NULL == outBuf || inSize < sizeof( dmsDictExtent )
           || inSize % DMS_PAGE_SIZE4K != 0 )
      {
         len = ossSnprintf( outBuf, outSize,
                            "Error: dumpDictExtent input size (%d) is too "
                            "small or not aligned with 4K"OSS_NEWLINE,
                            inSize ) ;
         goto exit ;
      }

      if ( extent->_eyeCatcher[0] != DMS_DICT_EXTENT_EYECATCHER0
           || extent->_eyeCatcher[1] != DMS_DICT_EXTENT_EYECATCHER1 )
      {
         len += ossSnprintf( outBuf + len, outSize - len,
                             "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
                             extent->_eyeCatcher[0],
                             extent->_eyeCatcher[1] ) ;
         goto exit ;
      }

      len += ossSnprintf( outBuf + len, outSize - len,
                          " ExtentId: 0x%08x (%d)"OSS_NEWLINE,
                          extID, extID ) ;
      if ( DMS_SU_DMP_OPT_HEX & options )
      {
         if ( DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR & options )
         {
            hexDumpOption |= OSS_HEXDUMP_PREFIX_AS_ADDR ;
         }
         if ( !(DMS_SU_DMP_OPT_HEX_WITH_ASCII & options ) )
         {
            hexDumpOption |= OSS_HEXDUMP_RAW_HEX_ONLY ;
         }
         len += ossHexDumpBuffer( inBuf, inSize, outBuf + len, outSize - len,
                                  addrPrefix, hexDumpOption ) ;
      }

      if ( DMS_SU_DMP_OPT_FORMATTED & options )
      {
         len += ossSnprintf( outBuf + len, outSize - len,
                         " Compression Dictionary Extent Header :"OSS_NEWLINE ) ;
         len += dumpExtentHeader( inBuf, inSize, outBuf + len, outSize - len ) ;
         if ( DMS_EXTENT_FLAG_FREED == extent->_flag )
         {
            len += ossSnprintf( outBuf + len, outSize - len,
                                "Error: Extent is not in use"OSS_NEWLINE ) ;
            goto exit ;
         }

         len += _dumpDictDetail( inBuf, inSize, outBuf + len, outSize - len ) ;
      }

      len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;
   exit:
      return len ;
   }

   UINT32 _dmsDump::dumpExtOptExtent( CHAR *inBuf, UINT32 inSize,
                                      CHAR *outBuf, UINT32 outSize,
                                      CHAR *addrPrefix, UINT32 options,
                                      dmsExtentID extID,
                                      DMS_STORAGE_TYPE type )
   {
      UINT32 len = 0 ;
      UINT32 hexDumpOption = 0 ;
      dmsOptExtent *extent = (dmsOptExtent *)inBuf ;

      if ( extent->_eyeCatcher[0] != DMS_OPT_EXTENT_EYECATCHER0
           || extent->_eyeCatcher[1] != DMS_OPT_EXTENT_EYECATCHER1 )
      {
         len += ossSnprintf( outBuf + len, outSize - len,
                             "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
                             extent->_eyeCatcher[0],
                             extent->_eyeCatcher[1] ) ;
         goto exit ;
      }

      len += ossSnprintf( outBuf + len, outSize - len,
                          " ExtentId: 0x%08x (%d)"OSS_NEWLINE,
                          extID, extID ) ;
      if ( DMS_SU_DMP_OPT_HEX & options )
      {
         if ( DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR & options )
         {
            hexDumpOption |= OSS_HEXDUMP_PREFIX_AS_ADDR ;
         }
         if ( !(DMS_SU_DMP_OPT_HEX_WITH_ASCII & options ) )
         {
            hexDumpOption |= OSS_HEXDUMP_RAW_HEX_ONLY ;
         }
         len += ossHexDumpBuffer( inBuf, inSize, outBuf + len, outSize - len,
                                  addrPrefix, hexDumpOption ) ;
      }
      if ( DMS_SU_DMP_OPT_FORMATTED & options )
      {
         len += ossSnprintf( outBuf + len, outSize - len,
                         " Extend Option Extent Header :"OSS_NEWLINE ) ;
         len += dumpExtentHeader( inBuf, inSize, outBuf + len, outSize - len ) ;
         if ( DMS_EXTENT_FLAG_FREED == extent->_flag )
         {
            len += ossSnprintf( outBuf + len, outSize - len,
                                "Error: Extent is not in use"OSS_NEWLINE ) ;
            goto exit ;
         }
         len += _dumpExtOptionDetail( inBuf, inSize, outBuf + len,
                                      outSize - len, type ) ;
      }
      len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;
   exit:
      return len ;
   }

   UINT32 _dmsDump::dumpDataExtent( pmdEDUCB *cb, CHAR *inBuf, UINT32 inSize,
                                    CHAR *outBuf, UINT32 outSize,
                                    CHAR *addrPrefix, UINT32 options,
                                    dmsExtentID &nextExtent,
                                    dmsCompressorEntry *compressorEntry,
                                    set< dmsRecordID > *ridList,
                                    BOOLEAN dumpRecord,
                                    BOOLEAN capped )
   {
      UINT32 len           = 0 ;
      UINT32 hexDumpOption = 0 ;
      dmsExtent *extent    = (dmsExtent*)inBuf ;

      if ( NULL == inBuf || NULL == outBuf || inSize < sizeof(dmsExtent) ||
           inSize % DMS_PAGE_SIZE4K != 0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpDataExtent input size (%d) "
                              "is too small or not aligned with 4K"OSS_NEWLINE,
                              inSize ) ;
         nextExtent = DMS_INVALID_EXTENT ;
         goto exit ;
      }

      if ( extent->_eyeCatcher[0] != DMS_EXTENT_EYECATCHER0 ||
           extent->_eyeCatcher[1] != DMS_EXTENT_EYECATCHER1 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
                              extent->_eyeCatcher[0], extent->_eyeCatcher[1] ) ;
         nextExtent = DMS_INVALID_EXTENT ;
         goto exit ;
      }

      if ( DMS_INVALID_EXTENT != nextExtent )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              " ExtentID: 0x%08x (%d)"OSS_NEWLINE,
                              nextExtent, nextExtent ) ;
      }
      nextExtent = extent->_nextExtent ;

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
         len += ossSnprintf ( outBuf + len, outSize -len,
                              " Data Extent Header:"OSS_NEWLINE ) ;
         len += dumpExtentHeader ( inBuf, inSize, outBuf + len,
                                   outSize - len ) ;
         if ( DMS_EXTENT_FLAG_FREED == extent->_flag )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Extent is not in use"OSS_NEWLINE ) ;
            goto exit ;
         }

         if( dumpRecord )
         {
            if ( capped )
            {
               len += _dumpCappedExtent( inBuf, inSize, outBuf + len,
                                         outSize - len, compressorEntry, cb ) ;
            }
            else
            {
               len += _dumpNormalExtent( inBuf, inSize, outBuf + len,
                                         outSize - len, compressorEntry,
                                         ridList, cb ) ;
            }
         }
      }

   exit :
      return len ;
   }

   UINT32 _dmsDump::dumpExtentHeader( void *inBuf, UINT32 inSize,
                                      CHAR *outBuf, UINT32 outSize )
   {
      UINT32 len           = 0 ;
      dmsExtent *extent    = (dmsExtent*)inBuf ;

      if ( NULL == inBuf || NULL == outBuf || inSize < sizeof(dmsExtent) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpExtentHeader input size (%d) "
                              "is too small"OSS_NEWLINE,
                              inSize ) ;
         goto exit ;
      }

      if ( DMS_EXTENT_EYECATCHER0 == extent->_eyeCatcher[0] &&
           DMS_EXTENT_EYECATCHER1 == extent->_eyeCatcher[1] )
      {
         return dumpDataExtentHeader ( inBuf, inSize, outBuf, outSize ) ;
      }
      else if ( DMS_META_EXTENT_EYECATCHER0 == extent->_eyeCatcher[0] &&
                DMS_META_EXTENT_EYECATCHER1 == extent->_eyeCatcher[1] )
      {
         return dumpMetaExtentHeader( inBuf, inSize, outBuf, outSize ) ;
      }
      else if ( DMS_DICT_EXTENT_EYECATCHER0 == extent->_eyeCatcher[0] &&
                DMS_DICT_EXTENT_EYECATCHER1 == extent->_eyeCatcher[1] )
      {
         return dumpDictExtentHeader( inBuf, inSize, outBuf, outSize ) ;
      }
      else if ( IXM_EXTENT_EYECATCHER0 == extent->_eyeCatcher[0] &&
                IXM_EXTENT_EYECATCHER1 == extent->_eyeCatcher[1] )
      {
         return dumpIndexExtentHeader ( inBuf, inSize, outBuf, outSize ) ;
      }
      else if ( IXM_EXTENT_CB_EYECATCHER0 == extent->_eyeCatcher[0] &&
                IXM_EXTENT_CB_EYECATCHER1 == extent->_eyeCatcher[1] )
      {
         return dumpIndexCBExtentHeader ( inBuf, inSize, outBuf, outSize ) ;
      }
      else if ( DMS_OPT_EXTENT_EYECATCHER0 == extent->_eyeCatcher[0] &&
                DMS_OPT_EXTENT_EYECATCHER1 == extent->_eyeCatcher[1] )
      {
         return dumpExtOptExtentHeader( inBuf, inSize, outBuf, outSize ) ;
      }
      else
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
                              extent->_eyeCatcher[0],
                              extent->_eyeCatcher[1] ) ;
      }

   exit :
      return len ;
   }

   UINT32 _dmsDump::_dumpExtentHeaderComm( const dmsExtent *extent,
                                           CHAR *outBuf, UINT32 outSize )
   {
      UINT32 len = 0 ;
      len += ossSnprintf ( outBuf, outSize,
                           "    Eye Catcher  : %c%c"OSS_NEWLINE,
                           extent->_eyeCatcher[0], extent->_eyeCatcher[1] ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Extent Size  : %u"OSS_NEWLINE,
                           extent->_blockSize ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    CollectionID : %u"OSS_NEWLINE,
                           extent->_mbID ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Flag         : 0x%02x (%s)"OSS_NEWLINE,
                           extent->_flag, extent->_flag==DMS_EXTENT_FLAG_INUSE ?
                           "InUse" : "Free" ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Version      : %d"OSS_NEWLINE,
                           extent->_version ) ;
      return len ;
   }

   UINT32 _dmsDump::dumpDataExtentHeader( void *inBuf, UINT32 inSize,
                                          CHAR *outBuf, UINT32 outSize )
   {
      UINT32 len           = 0 ;
      dmsExtent *extent    = (dmsExtent*)inBuf ;

      if ( NULL == inBuf || NULL == outBuf || inSize < sizeof(dmsExtent) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpExtentHeader input size (%d) "
                              "is too small"OSS_NEWLINE,
                              inSize ) ;
         goto exit ;
      }

      len += _dmsDump::_dumpExtentHeaderComm( (dmsExtent *)extent,
                                              outBuf, outSize ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Logic ID     : %d"OSS_NEWLINE,
                           extent->_logicID ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    PrevExtent   : 0x%08x (%d)"OSS_NEWLINE,
                           extent->_prevExtent, extent->_prevExtent ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    NextExtent   : 0x%08x (%d)"OSS_NEWLINE,
                           extent->_nextExtent, extent->_nextExtent ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Record Count : %u"OSS_NEWLINE,
                           extent->_recCount ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    First Record : 0x%08x (%d)"OSS_NEWLINE,
                           extent->_firstRecordOffset,
                           extent->_firstRecordOffset ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Last Record  : 0x%08x (%d)"OSS_NEWLINE,
                           extent->_lastRecordOffset,
                           extent->_lastRecordOffset ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Free Space   : %d"OSS_NEWLINE,
                           extent->_freeSpace ) ;

   exit :
      return len ;
   }

   UINT32 _dmsDump::dumpMetaExtentHeader( void * inBuf, UINT32 inSize,
                                          CHAR * outBuf, UINT32 outSize )
   {
      UINT32 len              = 0 ;
      dmsMetaExtent*extent    = (dmsMetaExtent*)inBuf ;

      if ( NULL == inBuf || NULL == outBuf || inSize < sizeof(dmsMetaExtent) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpExtentHeader input size (%d) "
                              "is too small"OSS_NEWLINE,
                              inSize ) ;
         goto exit ;
      }

      len += _dmsDump::_dumpExtentHeaderComm( (dmsExtent *)extent,
                                              outBuf, outSize ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Segment num  : %d"OSS_NEWLINE,
                           extent->_segNum ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Used seg num : %d"OSS_NEWLINE,
                           extent->_usedSegNum ) ;

   exit :
      return len ;
   }

   UINT32 _dmsDump::dumpDictExtentHeader( void *inBuf, UINT32 inSize,
                                          CHAR * outBuf, UINT32 outSize )
   {
      UINT32 len = 0 ;
      dmsDictExtent *extent = (dmsDictExtent *)inBuf ;

      if ( NULL == inBuf || NULL == outBuf
           ||  inSize < DMS_DICTEXTENT_HEADER_SZ)
      {
         len = ossSnprintf( outBuf, outSize,
                            "Error: dumpDictExtentHeader input size (%d) is "
                            "too small"OSS_NEWLINE, inSize ) ;
         goto exit ;
      }

      len += _dmsDump::_dumpExtentHeaderComm( (dmsExtent *)extent,
                                              outBuf, outSize ) ;
      len += ossSnprintf( outBuf + len, outSize - len,
                          "    Dictionary size: %u"OSS_NEWLINE,
                          extent->_dictLen ) ;
   exit:
      return len ;
   }

   UINT32 _dmsDump::dumpExtOptExtentHeader( void *inBuf, UINT32 inSize,
                                            CHAR * outBuf, UINT32 outSize )
   {
      UINT32 len = 0 ;
      dmsOptExtent *extent = (dmsOptExtent *)inBuf ;

      if ( NULL == inBuf || NULL == outBuf
           ||  inSize < DMS_DICTEXTENT_HEADER_SZ)
      {
         len = ossSnprintf( outBuf, outSize,
                            "Error: dumpExtOptExtentHeader input size (%d) "
                            "is too small"OSS_NEWLINE, inSize ) ;
         goto exit ;
      }

      len += _dmsDump::_dumpExtentHeaderComm( (dmsExtent *)extent,
                                              outBuf, outSize ) ;
      len += ossSnprintf( outBuf + len, outSize - len,
                          "    Extend option size: %u"OSS_NEWLINE,
                          extent->_optSize ) ;
   exit:
      return len ;
   }

   UINT32 _dmsDump::_dumpNormalExtent( CHAR *inBuf, UINT32 inSize,
                                       CHAR *outBuf, UINT32 outSize,
                                       dmsCompressorEntry *compressorEntry,
                                       set< dmsRecordID > *ridList,
                                       pmdEDUCB *cb )
   {
      UINT32 len = 0 ;
      dmsExtent *extent = (dmsExtent *)inBuf ;
      dmsOffset nextRecord = extent->_firstRecordOffset ;
      INT32 recordCount = 0 ;

      while ( DMS_INVALID_OFFSET != nextRecord && len < outSize )
      {
         if ( nextRecord >= (SINT32)inSize )
         {
            len += ossSnprintf (  outBuf + len, outSize - len,
                                  "Error : nextRecord %d is greater "
                                  "than inSize %d",
                                  nextRecord, inSize ) ;
            goto exit ;
         }
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "    Record %d:"OSS_NEWLINE,
                              recordCount ) ;
         len += dumpDataRecord ( cb, ((CHAR*)extent)+nextRecord,
                                 inSize - nextRecord,
                                 outBuf + len, outSize - len,
                                 nextRecord, compressorEntry,
                                 ridList) ;

         len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;
         ++recordCount ;
      }
   exit:
      return len ;
   }

   UINT32 _dmsDump::_dumpCappedExtent( CHAR *inBuf, UINT32 inSize,
                                       CHAR *outBuf, UINT32 outSize,
                                       dmsCompressorEntry *compressorEntry,
                                       pmdEDUCB *cb )
   {
      UINT32 len = 0 ;
      dmsExtent *extent = (dmsExtent *)inBuf ;
      dmsOffset nextRecord = extent->_firstRecordOffset ;
      dmsOffset lastRecord = extent->_lastRecordOffset ;
      INT32 recordCount = 0 ;

      if ( NULL == inBuf || NULL == outBuf || inSize < sizeof(dmsCappedRecord) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpCappedDataRecord input size (%d) "
                              "is too small"OSS_NEWLINE,
                              inSize ) ;
         nextRecord = DMS_INVALID_OFFSET ;
         goto exit ;
      }

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
            goto exit ;
         }

         record = (dmsCappedRecord*)( ((CHAR*)inBuf) + nextRecord ) ;
         logicalID = record->getLogicalID() ;

         if ( logicalID < 0 )
         {
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

         len += ossSnprintf ( outBuf + len, outSize - len,
                              "    Record %d:"OSS_NEWLINE,
                              recordCount ) ;

         len += dumpCappedDataRecord ( cb, record,
                                       outBuf + len, outSize - len,
                                       compressorEntry ) ;

         if ( DMS_INVALID_OFFSET != nextRecord )
         {
            nextRecord = ossRoundUpToMultipleX( nextRecord + record->getSize(),
                                                4 ) ;
         }

         len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;
         ++recordCount ;
      }
   exit:
      return len ;
   }

   #define DMS_DUMP_DATA_RECORD_FLAG_TEXT_LEN         63

   UINT32 _dmsDump::dumpDataRecord( pmdEDUCB *cb, CHAR *inBuf, UINT32 inSize,
                                    CHAR *outBuf, UINT32 outSize,
                                    dmsOffset &nextRecord,
                                    dmsCompressorEntry *compressorEntry,
                                    set< dmsRecordID > *ridList )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( cb, "cb can't be NULL" ) ;
      UINT32 len        = 0 ;
      dmsRecord *record = (dmsRecord*)inBuf ;

      CHAR flag         = 0 ;
      UINT32 recordSize = 0 ;

      CHAR      flagText [DMS_DUMP_DATA_RECORD_FLAG_TEXT_LEN+1] = {0} ;
      BOOLEAN   isOvf   = FALSE ;
      BOOLEAN   isDel   = FALSE ;

      if ( NULL == inBuf || NULL == outBuf || inSize < sizeof(dmsRecord) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpDataRecord input size (%d) "
                              "is too small"OSS_NEWLINE,
                              inSize ) ;
         nextRecord = DMS_INVALID_OFFSET ;
         goto exit ;
      }

      flag       = record->getFlag() ;
      recordSize = record->getSize() ;

      if ( record->isNormal() )
      {
         ossStrncat( flagText, "Normal", DMS_DUMP_DATA_RECORD_FLAG_TEXT_LEN ) ;
      }
      if ( record->isOvf() )
      {
         appendString( flagText, DMS_DUMP_DATA_RECORD_FLAG_TEXT_LEN,
                       "OvfFrom" ) ;
         isOvf = TRUE ;
      }
      if ( record->isOvt() )
      {
         appendString( flagText, DMS_DUMP_DATA_RECORD_FLAG_TEXT_LEN,
                       "OvfTo" ) ;
      }
      if ( record->isDeleted() )
      {
         isDel = TRUE ;
         appendString( flagText, DMS_DUMP_DATA_RECORD_FLAG_TEXT_LEN,
                       "Deleted" ) ;
      }
      if ( record->isDeleting() )
      {
         appendString( flagText, DMS_DUMP_DATA_RECORD_FLAG_TEXT_LEN,
                       "Deleting" ) ;
      }

      len += ossSnprintf ( outBuf + len, outSize - len,
                           "       Flag        : 0x%02x (%s)"OSS_NEWLINE,
                           flag, flagText ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "       Compressed  : %s"OSS_NEWLINE,
                           OSS_BIT_TEST ( flag, DMS_RECORD_FLAG_COMPRESSED ) ?
                           "True":"False" ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "       Record Size : %u"OSS_NEWLINE,
                           recordSize ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "       My Offset   : 0x%08x (%d)"OSS_NEWLINE,
                           record->_myOffset, record->_myOffset ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "       Prev Offset : 0x%08x (%d)"OSS_NEWLINE,
                           record->_previousOffset, record->_previousOffset ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "       Next Offset : 0x%08x (%d)"OSS_NEWLINE,
                           record->_nextOffset, record->_nextOffset ) ;

      if ( DMS_INVALID_OFFSET != nextRecord &&  record->_myOffset!= nextRecord )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: my offset (0x%08x) does not match "
                              "expected ( 0x%08x)"OSS_NEWLINE,
                              record->_myOffset, nextRecord ) ;
         nextRecord = DMS_INVALID_OFFSET ;
         goto exit ;
      }

      nextRecord = record->_nextOffset ;
      if ( isDel )
      {
         nextRecord = DMS_INVALID_OFFSET ;
         goto exit ;
      }
      else if ( isOvf )
      {
         dmsRecordID rid = record->getOvfRID() ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "       Overflowed To: 0x%08x : 0x%08x ( "
                              "extent %d offset %d )"OSS_NEWLINE,
                              rid._extent, rid._offset, rid._extent,
                              rid._offset ) ;
         if ( ridList )
         {
            ridList->insert ( rid ) ;
         }
      }
      else
      {
         try
         {
            ossValuePtr recordPtr = 0 ;
            DMS_RECORD_EXTRACTDATA ( record, recordPtr,
                                     compressorEntry ) ;
            BSONObj obj ( (CHAR*)recordPtr ) ;
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "       Record: %s"OSS_NEWLINE,
                                 obj.toString( FALSE, TRUE ).c_str() ) ;
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

   UINT32 _dmsDump::dumpCappedDataRecord( pmdEDUCB *cb, dmsCappedRecord *record,
                                          CHAR *outBuf,
                                          UINT32 outSize,
                                          dmsCompressorEntry *compressorEntry )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( cb, "cb can't be NULL" ) ;
      UINT32 len = 0 ;
      CHAR flag = 0 ;
      CHAR flagText [DMS_DUMP_DATA_RECORD_FLAG_TEXT_LEN+1] = {0} ;

      flag = record->getFlag() ;
      if ( record->isNormal() && record->getLogicalID() >= 0 )
      {
         ossStrncat( flagText, "Normal", DMS_DUMP_DATA_RECORD_FLAG_TEXT_LEN ) ;
      }

      len += ossSnprintf ( outBuf + len, outSize - len,
                           "       Flag        : 0x%02x (%s)"OSS_NEWLINE,
                           flag, flagText ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "       Compressed  : %s"OSS_NEWLINE,
                           OSS_BIT_TEST ( flag, DMS_RECORD_FLAG_COMPRESSED ) ?
                           "True":"False" ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "       Record Size : %u"OSS_NEWLINE,
                           record->getSize() ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "       Record Number: %u"OSS_NEWLINE,
                           record->getRecordNo() ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "       LogicalID: %lld"OSS_NEWLINE,
                           record->getLogicalID() ) ;

      try
      {
         ossValuePtr recordPtr = 0 ;
         DMS_RECORD_EXTRACTDATA ( record, recordPtr,
                                  compressorEntry ) ;
         BSONObj obj ( (CHAR*)recordPtr ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "       Record: %s"OSS_NEWLINE,
                              obj.toString( FALSE, TRUE ).c_str() ) ;
      }
      catch ( std::exception &e )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Failed to format "
                              "record: %s"OSS_NEWLINE,
                              e.what() ) ;
      }

   exit:
      return len ;
   error:
      goto exit ;
   }

   UINT32 _dmsDump::dumpIndexExtent( void *inBuf, UINT32 inSize,
                                     CHAR *outBuf, UINT32 outSize,
                                     CHAR *addrPrefix, UINT32 options,
                                     deque< dmsExtentID > &childExtents,
                                     BOOLEAN dumpIndexKey )
   {
      UINT32 len           = 0 ;
      UINT32 hexDumpOption = 0 ;
      ixmExtentHead *extentHead = (ixmExtentHead*)inBuf ;

      if ( NULL == inBuf || NULL == outBuf || inSize < sizeof(ixmExtentHead) ||
           inSize % DMS_PAGE_SIZE4K != 0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpIndexExtent input size (%d) "
                              "is too small or not aligned with 4K"OSS_NEWLINE,
                              inSize ) ;
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

      for ( INT32 i = 0 ; i < extentHead->_totalKeyNodeNum ; ++i )
      {
         UINT32 keyOffset = sizeof(ixmExtentHead) + sizeof(ixmKeyNode)*i ;
         ixmKeyNode *key = (ixmKeyNode*)(((CHAR*)inBuf)+keyOffset) ;
         if ( keyOffset > inSize )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: key offset is out of range: %d, "
                                 "extent size: %d, key pos: %d"OSS_NEWLINE,
                                 keyOffset, inSize, i ) ;
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
         len += ossSnprintf ( outBuf + len, outSize -len,
                              " Index Extent Header:"OSS_NEWLINE ) ;
         len += dumpExtentHeader ( inBuf, inSize, outBuf + len,
                                   outSize - len ) ;
         if ( DMS_EXTENT_FLAG_FREED == extentHead->_flag )
         {
            len += ossSnprintf ( outBuf + len, outSize - len,
                                 "Error: Extent is not in use"OSS_NEWLINE ) ;
            goto exit ;
         }

         if( dumpIndexKey )
         {
            for ( INT32 i = 0; i < extentHead->_totalKeyNodeNum; ++i )
            {
               UINT32 keyOffset = sizeof(ixmExtentHead) +
                                  sizeof(ixmKeyNode)*i ;
               if ( keyOffset > inSize )
               {
                  len += ossSnprintf ( outBuf + len, outSize - len,
                                       "Error: key offset is out of range: %d, "
                                       "extent size: %d, key pos: %d"OSS_NEWLINE,
                                       keyOffset, inSize, i ) ;
                  goto exit ;
               }
               len += ossSnprintf ( outBuf + len, outSize - len,
                                    "    Key %d:"OSS_NEWLINE,
                                    i ) ;
               len += dumpIndexRecord ( ((CHAR*)inBuf),
                                        inSize,
                                        outBuf + len, outSize - len,
                                        keyOffset ) ;
               len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;
            } // for ( INT32 i = 0; i < extentHead->_totalKeyNodeNum; ++i )
         }
      } // if ( DMS_SU_DMP_OPT_FORMATTED & options )

   exit :
      return len ;
   }

   UINT32 _dmsDump::dumpIndexExtentHeader( void *inBuf, UINT32 inSize,
                                           CHAR *outBuf, UINT32 outSize )
   {
      UINT32 len           = 0 ;
      ixmExtentHead *header=(ixmExtentHead*)inBuf ;

      if ( NULL == inBuf || NULL == outBuf || inSize < sizeof(ixmExtentHead) )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpIndexExtentHeader input size (%d) "
                              "is too small"OSS_NEWLINE,
                              inSize ) ;
         goto exit ;
      }

      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Eye Catcher  : %c%c"OSS_NEWLINE,
                           header->_eyeCatcher[0], header->_eyeCatcher[1] ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Total Keys   : %u"OSS_NEWLINE,
                           header->_totalKeyNodeNum ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    CollectionID : %u"OSS_NEWLINE,
                           header->_mbID ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Flag         : 0x%02x (%s)"OSS_NEWLINE,
                           header->_flag, header->_flag==DMS_EXTENT_FLAG_INUSE ?
                           "InUse" : "Free" ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Version      : %d"OSS_NEWLINE,
                           header->_version ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Parent Ext   : 0x%08x (%d)%s"OSS_NEWLINE,
                           header->_parentExtentID , header->_parentExtentID,
                           DMS_INVALID_EXTENT == header->_parentExtentID ?
                           " (root)":"" ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Free Offset  : 0x%08x (%d)"OSS_NEWLINE,
                           header->_beginFreeOffset,
                           header->_beginFreeOffset ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Total Free   : 0x%08x (%d)"OSS_NEWLINE,
                           header->_totalFreeSize, header->_totalFreeSize ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Right Child  : 0x%08x (%d)"OSS_NEWLINE,
                           header->_right, header->_right ) ;

   exit :
      return len ;
   }

   UINT32 _dmsDump::dumpIndexRecord( void *inBuf, UINT32 inSize,
                                     CHAR *outBuf, UINT32 outSize,
                                     UINT32 keyOffset )
   {
      UINT32 len = 0 ;
      dmsExtentID left ;
      dmsRecordID rid ;
      UINT16 keyOfst ;
      ixmKeyNode *keyNode = NULL ;

      if ( keyOffset > inSize )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: key offset is out of range: %u"
                              OSS_NEWLINE, keyOffset ) ;
         goto exit ;
      }

      keyNode   = (ixmKeyNode*)(((CHAR*)inBuf)+keyOffset ) ;
      left      = keyNode->_left ;
      rid       = keyNode->_rid ;
      keyOfst   = keyNode->_keyOffset ;

      len += ossSnprintf ( outBuf + len, outSize - len,
                           "       Left Ptr     : 0x%08x (%d)"OSS_NEWLINE,
                           left, left ) ;
      if ( rid._offset & 1 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "       Record ID    : 0x%08x : 0x%08x "
                              "(Unused)"OSS_NEWLINE,
                              rid._extent, rid._offset ) ;
      }
      else
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "       Record ID    : 0x%08x : 0x%08x "
                              "(extent: %d; offset: %d)"OSS_NEWLINE,
                              rid._extent, rid._offset, rid._extent,
                              rid._offset ) ;
      }
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "       Key Offset   : 0x%04x (%u)"OSS_NEWLINE,
                           keyOfst, keyOfst ) ;
      if ( keyOfst > inSize )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Key Offset is out of range: %u"
                              OSS_NEWLINE, keyOfst ) ;
         goto exit ;
      }

      try
      {
         ixmKey key ( ((CHAR*)inBuf)+keyOfst ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "       Key Value    : %s"OSS_NEWLINE,
                              key.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Failed to extract key: %s"
                              OSS_NEWLINE, e.what() ) ;
      }

   exit :
      return len ;
   }

   #define DMS_DUMP_IXM_CB_FLAG_TEXT_LEN        63

   UINT32 _dmsDump::dumpIndexCBExtentHeader( void *inBuf, UINT32 inSize,
                                             CHAR *outBuf, UINT32 outSize )
   {
      UINT32 len           = 0 ;
      ixmIndexCBExtent *header = (ixmIndexCBExtent*)inBuf ;
      CHAR tmpBuff [ DMS_DUMP_IXM_CB_FLAG_TEXT_LEN + 1 ] = {0} ;

      if ( NULL == inBuf || NULL == outBuf ||
           inSize < sizeof(ixmIndexCBExtent) || inSize % DMS_PAGE_SIZE4K != 0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpIndexCBExtentHeader input size (%d) "
                              "is too small or not aligned with 4K"OSS_NEWLINE,
                              inSize ) ;
         goto exit ;
      }

      if ( header->_eyeCatcher[0] != IXM_EXTENT_CB_EYECATCHER0 ||
           header->_eyeCatcher[1] != IXM_EXTENT_CB_EYECATCHER1 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
                              header->_eyeCatcher[0], header->_eyeCatcher[1] ) ;
         goto exit ;
      }

      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Eye Catcher  : %c%c"OSS_NEWLINE,
                           header->_eyeCatcher[0], header->_eyeCatcher[1] ) ;

      ossStrncpy ( tmpBuff, getIndexFlagDesp(header->_indexFlag),
                   DMS_DUMP_IXM_CB_FLAG_TEXT_LEN ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Index Flags  : %d (%s)"OSS_NEWLINE,
                           header->_indexFlag, tmpBuff ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    CollectionID : %u"OSS_NEWLINE,
                           header->_mbID ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Flag         : 0x%02x (%s)"OSS_NEWLINE,
                           header->_flag, header->_flag==DMS_EXTENT_FLAG_INUSE ?
                           "InUse" : "Free" ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Version      : %d"OSS_NEWLINE,
                           header->_version ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "  Logical ID     : %d"OSS_NEWLINE,
                           header->_logicID ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "    Root Ext    : 0x%08x (%d)"OSS_NEWLINE,
                           header->_rootExtentID, header->_rootExtentID ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "Scan extent LID : 0x%08x (%d)"OSS_NEWLINE,
                           header->_scanExtLID, header->_scanExtLID ) ;
      ossStrncpy ( tmpBuff, getIndexTypeDesp(header->_type).c_str(),
                   DMS_DUMP_IXM_CB_FLAG_TEXT_LEN ) ;
      len += ossSnprintf ( outBuf + len, outSize - len,
                           "     Type       : %d (%s)"OSS_NEWLINE,
                           header->_type, tmpBuff ) ;
      try
      {
         BSONObj indexDef ( ((CHAR*)inBuf+sizeof(ixmIndexCBExtent)) ) ;
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "    Index Def   : %s"OSS_NEWLINE,
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
      UINT32 len           = 0 ;
      UINT32 hexDumpOption = 0 ;
      ixmIndexCBExtent *extent = (ixmIndexCBExtent*)inBuf ;

      if ( NULL == inBuf || NULL == outBuf ||
           inSize < sizeof(ixmIndexCBExtent) || inSize % DMS_PAGE_SIZE4K != 0 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: dumpIndexCBExtent input size (%d) "
                              "is too small or not aligned with 4K"OSS_NEWLINE,
                              inSize ) ;
         goto exit ;
      }

      if ( extent->_eyeCatcher[0] != IXM_EXTENT_CB_EYECATCHER0 ||
           extent->_eyeCatcher[1] != IXM_EXTENT_CB_EYECATCHER1 )
      {
         len += ossSnprintf ( outBuf + len, outSize - len,
                              "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
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


UINT32 _dmsDump::dumpDmsLobMeta( CHAR *inBuf, UINT32 inSize,
                                 CHAR * outBuf,UINT32 outSize,
                                 CHAR * addrPrefix, UINT32 options )
{
   UINT32 len           = 0 ;
   UINT32 hexDumpOption = 0 ;

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
      len += ossHexDumpBuffer(inBuf, inSize, outBuf+len, outSize-len,
                            addrPrefix, hexDumpOption ) ;
   }


   if ( DMS_SU_DMP_OPT_FORMATTED & options )
   {
      dmsLobMeta *lobMeta = (dmsLobMeta*)inBuf;
      const char *tag = NULL;
      len += ossSnprintf(outBuf + len, outSize -len, "Lobd Meta:"OSS_NEWLINE) ;

      len += ossSnprintf(outBuf + len, outSize - len,
                                    " Lob Len        :%lld"OSS_NEWLINE,
                                    lobMeta->_lobLen);

      CHAR strTime[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      ossTimestamp tm(lobMeta->_createTime);
      ossTimestampToString(tm , strTime ) ;
      len += ossSnprintf(outBuf + len, outSize - len,
                                    " Create Time    :%s (%llu)"OSS_NEWLINE,
                                    strTime, lobMeta->_createTime) ;

      tag = lobMeta->isDone()? "DMS_LOB_COMPLETE":"DMS_LOB_UNCOMPLETE";
      len += ossSnprintf(outBuf + len, outSize - len,
                                    " Status         :%s (%u)"OSS_NEWLINE,
                                    tag, lobMeta->_status);

      tag = (lobMeta->_version == DMS_LOB_META_CURRENT_VERSION )
                          ? "DMS_LOB_META_CURRENT_VERSION"
                          : NULL;

      len += ossSnprintf(outBuf + len, outSize - len,
                                    " Version        :%s (%u)"OSS_NEWLINE,
                                    tag, lobMeta->_version) ;

      tm = lobMeta->_createTime;
      ossTimestampToString(tm , strTime ) ;
      len += ossSnprintf(outBuf + len, outSize - len,
                                    " Mod Time       :%s (%llu)"OSS_NEWLINE,
                                    strTime, lobMeta->_modificationTime);

      tag = lobMeta->hasPiecesInfo()
                       ? "DMS_LOB_META_FLAG_PIECESINFO_INSIDE"
                       : "NO PIECESINFO";

      len += ossSnprintf(outBuf + len, outSize - len, " Flag           :%s (%u)"OSS_NEWLINE, tag, lobMeta->_flag);

      len += ossSnprintf(outBuf + len,
                                   outSize - len,
                                   " PiecesInfo Num :%d"OSS_NEWLINE,
                                   lobMeta->_piecesInfoNum);

      if ( (lobMeta->_piecesInfoNum <=  0) ||
            (lobMeta->_piecesInfoNum > (INT32)(DMS_LOB_META_LENGTH /sizeof( _rtnLobPieces ) )) )
         goto exit;

      len += ossSnprintf(outBuf + len, outSize - len, " Pieces:");
      _rtnLobPieces* piecesInfoBuf = (_rtnLobPieces*)(inBuf + DMS_LOB_META_LENGTH
                               - sizeof( _rtnLobPieces ) * lobMeta->_piecesInfoNum);
      for(INT32 i = 0; i < lobMeta->_piecesInfoNum; i ++)
      {
         len += ossSnprintf ( outBuf+len, outSize-len,
                                          "      { first:%u; last:%u }"OSS_NEWLINE,
                                          piecesInfoBuf[i].first,
                                          piecesInfoBuf[i].last);
      }
   }

   exit :
   len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;
   return len ;

}

UINT32 _dmsDump::dumpDmsLobData( CHAR *inBuf, UINT32 inSize,
                                 CHAR * outBuf, UINT32 outSize,
                                 CHAR * addrPrefix, UINT32 options )
{
   UINT32 len           = 0 ;
   UINT32 hexDumpOption = 0 ;

   len += ossSnprintf(outBuf + len, outSize -len, "Lobd Data:") ;

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
      len += ossHexDumpBuffer(inBuf, inSize, outBuf+len, outSize-len,
                            addrPrefix, hexDumpOption ) ;
   }


   if ( DMS_SU_DMP_OPT_FORMATTED & options )
   {
   }

   len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;
   return len ;

}


UINT32 _dmsDump::dumpDmsLobDataMapBlk(dmsLobDataMapBlk *blk, CHAR * outBuf,
                              UINT32 outSize, CHAR * addrPrefix,
                              UINT32 options, UINT32 pageSize)
{
   UINT32 len           = 0 ;
   UINT32 hexDumpOption = 0 ;

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
      len += ossHexDumpBuffer(blk, pageSize, outBuf+len, outSize-len,
                         addrPrefix, hexDumpOption ) ;
   }

   if ( DMS_SU_DMP_OPT_FORMATTED & options )
   {
      const char *tag = NULL;
      len += ossSnprintf(outBuf + len, outSize -len, "Lobm dmsLobDataMapBlk:"OSS_NEWLINE);
      bson::OID oid;
      ossMemcpy(&oid, blk->_oid, DMS_LOB_OID_LEN);
      len += ossSnprintf(outBuf + len, outSize -len,  " Oid            :%s"OSS_NEWLINE, oid.str().c_str());
      len += ossSnprintf(outBuf + len, outSize - len, " Sequence       :%u"OSS_NEWLINE, blk->_sequence);
      len += ossSnprintf(outBuf + len, outSize - len, " Data Len       :%u"OSS_NEWLINE, blk->_dataLen);
      len += ossSnprintf(outBuf + len, outSize - len, " Prev PageId    :%d"OSS_NEWLINE, blk->_prevPageInBucket);
      len += ossSnprintf(outBuf + len, outSize - len, " Next PageId    :%d"OSS_NEWLINE, blk->_nextPageInBucket);
      len += ossSnprintf(outBuf + len, outSize - len, " CL LogicId     :%u"OSS_NEWLINE, blk->_clLogicalID);
      len += ossSnprintf(outBuf + len, outSize - len, " MB Id          :%u"OSS_NEWLINE, blk->_mbID);

      tag = blk->isNormal()? "DMS_LOB_PAGE_NORMAL":"DMS_LOB_PAGE_REMOVED";
      len += ossSnprintf(outBuf + len, outSize - len, " Status         :%s (%u)"OSS_NEWLINE,tag, blk->_status);
   }

   len += ossSnprintf ( outBuf + len, outSize - len, OSS_NEWLINE ) ;
   return len ;
}


}


