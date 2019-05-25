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

   Source File Name = dmsReorgUnit.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "dmsReorgUnit.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsRecord.hpp"
#include "ossIO.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmdEDU.hpp"
#include "dmsCompress.hpp"

using namespace bson ;

namespace engine
{
   #define DMS_REORG_UNIT_HEAD_SIZE_UNIT           ( 1024 )

   _dmsReorgUnit::_dmsReorgUnit ()
   {
      _pCurrentExtent = NULL ;
      _currentExtentSize = 0 ;
      _buffSize = 0 ;
      ossMemset ( _fileName, 0, sizeof(_fileName) ) ;
      _pageSize = 0 ;
      _headSize = 0 ;
      _readOnly = TRUE ;
   }

   _dmsReorgUnit::~_dmsReorgUnit ()
   {
      close() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSROUNIT__INIT, "_dmsReorgUnit::_init" )
   INT32 _dmsReorgUnit::_init( BOOLEAN createNew )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSROUNIT__INIT ) ;

      _reorgUnitHead *unitHead = NULL ;
      _headSize = ossRoundUpToMultipleX ( sizeof ( _reorgUnitHead ),
                                          DMS_REORG_UNIT_HEAD_SIZE_UNIT ) ;

      CHAR *pBuffer = (CHAR*)SDB_OSS_MALLOC( _headSize ) ;
      if ( !pBuffer )
      {
         PD_LOG ( PDERROR, "Failed to allocate %d bytes of memory",
                  _headSize ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      unitHead = (_reorgUnitHead*)pBuffer ;
      ossMemset ( unitHead, 0, _headSize ) ;

      if ( createNew )
      {
         _readOnly = FALSE ;

         ossMemcpy ( unitHead->_eyeCatcher, DMS_REORG_UNIT_EYECATCHER,
                     DMS_REORG_UNIT_EYECATCHER_LEN ) ;
         unitHead->_headerSize = _headSize ;
         ossMemcpy ( unitHead->_fileName, _fileName, OSS_MAX_PATHSIZE ) ;
         unitHead->_pageSize = _pageSize ;

         rc = ossWriteN( &_file, pBuffer, _headSize ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to write header to file, rc: %d", rc ) ;
            goto error ;
         }
      }
      else
      {
         _readOnly = TRUE ;

         INT64 read = 0 ;
         rc = ossReadN( &_file, _headSize, pBuffer, read ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to read header from file, rc: %d", rc ) ;
            goto error ;
         }

         if ( ossMemcmp ( unitHead->_eyeCatcher, DMS_REORG_UNIT_EYECATCHER,
                          DMS_REORG_UNIT_EYECATCHER_LEN ) ||
              unitHead->_headerSize != _headSize ||
              unitHead->_pageSize != _pageSize )
         {
            PD_LOG ( PDERROR, "Invalid reorg file is detected" ) ;
            rc = SDB_DMS_INVALID_REORG_FILE ;
            goto error ;
         }
      }

   done :
      if ( pBuffer )
      {
         SDB_OSS_FREE ( pBuffer ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSROUNIT__INIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSROUNIT_CLNUP, "_dmsReorgUnit::cleanup" )
   INT32 _dmsReorgUnit::cleanup ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSROUNIT_CLNUP );
      close() ;
      rc = ossDelete ( _fileName ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to delete reorg unit temp file, rc = %d",
                  rc ) ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__DMSROUNIT_CLNUP, rc );
      return rc ;
   error :
      goto done ;
   }

   BOOLEAN _dmsReorgUnit::isOpened() const
   {
      return _file.isOpened() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSROUNIT_OPEN, "_dmsReorgUnit::open" )
   INT32 _dmsReorgUnit::open ( const CHAR *pFileName,
                               SINT32 pageSize,
                               BOOLEAN createNew )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSROUNIT_OPEN );

      ossStrncpy ( _fileName, pFileName, OSS_MAX_PATHSIZE ) ;
      _pageSize = pageSize ;

      rc = ossOpen ( _fileName, OSS_READWRITE|
                     (createNew?OSS_CREATEONLY:OSS_DEFAULT),
                     OSS_RU|OSS_WU, _file ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to create file %s, rc = %d",
                  _fileName, rc ) ;
         goto error ;
      }
      rc = _init ( createNew ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to initialize file %s, rc = %d",
                  _fileName, rc ) ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__DMSROUNIT_OPEN, rc );
      return rc ;
   error :
      goto done ;
   }

   void _dmsReorgUnit::close ()
   {
      ossClose ( _file ) ;

      if ( _pCurrentExtent )
      {
         SDB_OSS_FREE( _pCurrentExtent ) ;
         _pCurrentExtent = NULL ;
         _buffSize = 0 ;
         _currentExtentSize = 0 ;
      }
   }

   void _dmsReorgUnit::beginExport()
   {
      ossSeek( &_file, _headSize, OSS_SEEK_SET ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSROUNIT__ALCEXT, "_dmsReorgUnit::_allocateExtent" )
   INT32 _dmsReorgUnit::_allocateExtent ( INT32 requestSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSROUNIT__ALCEXT ) ;

      if ( requestSize < DMS_MIN_EXTENT_SZ(_pageSize) )
      {
         requestSize = DMS_MIN_EXTENT_SZ(_pageSize) ;
      }
      else if ( requestSize > DMS_MAX_EXTENT_SZ )
      {
         requestSize = DMS_MAX_EXTENT_SZ ;
      }
      else
      {
         requestSize = ossRoundUpToMultipleX ( requestSize, _pageSize ) ;
      }

      if ( requestSize > _buffSize && _pCurrentExtent )
      {
         SDB_OSS_FREE( _pCurrentExtent ) ;
         _pCurrentExtent = NULL ;
         _buffSize = 0 ;
      }

      if ( requestSize > _buffSize )
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
      PD_TRACE_EXITRC ( SDB__DMSROUNIT__ALCEXT, rc );
      return rc ;
   error :
      goto done ;
   }

   void _dmsReorgUnit::_initExtentHeader ( dmsExtent *extAddr, UINT16 numPages )
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSROUNIT__FLSEXT, "_dmsReorgUnit::_flushExtent" )
   INT32 _dmsReorgUnit::_flushExtent ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSROUNIT__FLSEXT );
      SDB_ASSERT ( _pCurrentExtent, "current extent can't be NULL" ) ;

      if ( _readOnly )
      {
         PD_LOG ( PDERROR, "Modify is not allowed" ) ;
         rc = SDB_DMS_REORG_FILE_READONLY ;
         goto error ;
      }

      rc = ossWriteN( &_file, _pCurrentExtent, _currentExtentSize ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to flush extent into file: %s, rc: %d",
                 _fileName, rc ) ;
         goto error ;
      }

      _currentExtentSize = 0 ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSROUNIT__FLSEXT, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSROUNIT_FLUSH, "_dmsReorgUnit::flush" )
   INT32 _dmsReorgUnit::flush ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSROUNIT_FLUSH );
      if ( _pCurrentExtent )
      {
         rc = _flushExtent () ;
      }
      PD_TRACE_EXITRC ( SDB__DMSROUNIT_FLUSH, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSROUNIT_INSRCD, "_dmsReorgUnit::insertRecord" )
   INT32 _dmsReorgUnit::insertRecord ( BSONObj &obj,
                                       _pmdEDUCB *cb,
                                       dmsCompressorEntry *compressorEntry )
   {
      INT32 rc                     = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSROUNIT_INSRCD );
      UINT32 dmsrecordSize         = 0 ;
      dmsRecord *pRecord           = NULL ;
      dmsRecord *pPrevRecord       = NULL ;
      dmsOffset offset             = DMS_INVALID_OFFSET ;
      dmsOffset recordOffset       = DMS_INVALID_OFFSET ;
      dmsExtent *currentExtent     = (dmsExtent*)_pCurrentExtent ;
      dmsRecordData recordData ;

      if ( obj.objsize() + DMS_RECORD_METADATA_SZ >
           DMS_RECORD_MAX_SZ )
      {
         rc = SDB_CORRUPTED_RECORD ;
         goto error ;
      }

      recordData.setData( obj.objdata(), obj.objsize(),
                          FALSE, TRUE ) ;
      dmsrecordSize = recordData.len() ;

      if ( compressorEntry->ready() )
      {
         const CHAR *compressedData    = NULL ;
         INT32 compressedDataSize      = 0 ;
         UINT8 compressRatio           = 0 ;
         rc = dmsCompress( cb, compressorEntry,
                           recordData.data(), recordData.len(),
                           &compressedData, &compressedDataSize,
                           compressRatio ) ;
         if ( SDB_OK == rc &&
              compressedDataSize + sizeof(UINT32) < recordData.orgLen() &&
              compressRatio < DMS_COMPRESS_RATIO_THRESHOLD )
         {
            dmsrecordSize = compressedDataSize + sizeof(UINT32) ;
            recordData.setData( compressedData, compressedDataSize,
                                TRUE, FALSE ) ;
         }
         else if ( rc )
         {
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

      dmsrecordSize *= DMS_RECORD_OVERFLOW_RATIO ;
      dmsrecordSize += DMS_RECORD_METADATA_SZ ;
      dmsrecordSize = OSS_MIN( DMS_RECORD_MAX_SZ,
                               ossAlignX( dmsrecordSize, 4 ) ) ;

   alloc:
      if ( 0 == _currentExtentSize )
      {
         INT32 expandSize = dmsrecordSize << DMS_RECORDS_PER_EXTENT_SQUARE ;
         if ( expandSize > DMS_BEST_UP_EXTENT_SZ )
         {
            expandSize = expandSize < DMS_BEST_UP_EXTENT_SZ ?
                         DMS_BEST_UP_EXTENT_SZ : expandSize ;
         }
         rc = _allocateExtent ( expandSize ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to allocate new extent in reorg file, "
                     "rc = %d", rc ) ;
            goto error ;
         }
         currentExtent = (dmsExtent*)_pCurrentExtent ;
      }
      if ( dmsrecordSize > (UINT32)currentExtent->_freeSpace )
      {
         rc = _flushExtent() ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to flush extent, rc = %d", rc ) ;
            goto error ;
         }
         goto alloc ;
      }
      recordOffset = _currentExtentSize - currentExtent->_freeSpace ;
      pRecord = (dmsRecord*)( (const CHAR*)currentExtent + recordOffset ) ;

      if ( currentExtent->_freeSpace - (INT32)dmsrecordSize <
           (INT32)DMS_MIN_RECORD_SZ &&
           currentExtent->_freeSpace <= (INT32)DMS_RECORD_MAX_SZ )
      {
         dmsrecordSize = (UINT32)currentExtent->_freeSpace ;
      }

      pRecord->setNormal() ;
      pRecord->resetAttr() ;
      pRecord->setMyOffset( recordOffset ) ;
      pRecord->setSize( dmsrecordSize ) ;
      pRecord->setData( recordData ) ;
      pRecord->setNextOffset( DMS_INVALID_OFFSET ) ;
      pRecord->setPrevOffset( DMS_INVALID_OFFSET ) ;

      currentExtent->_recCount++ ;
      currentExtent->_freeSpace -= dmsrecordSize ;

      offset = currentExtent->_lastRecordOffset ;
      if ( DMS_INVALID_OFFSET != offset )
      {
         pPrevRecord = ( dmsRecord *)( (const CHAR*)currentExtent + offset ) ;
         pPrevRecord->setNextOffset( recordOffset ) ;
         pRecord->setPrevOffset( offset ) ;
      }
      currentExtent->_lastRecordOffset = recordOffset ;
      offset = currentExtent->_firstRecordOffset ;
      if ( DMS_INVALID_OFFSET == offset )
      {
         currentExtent->_firstRecordOffset = recordOffset ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSROUNIT_INSRCD, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSROUNIT_GETNXTEXTSIZE, "_dmsReorgUnit::getNextExtentSize" )
   INT32 _dmsReorgUnit::getNextExtentSize( SINT32 &size )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSROUNIT_GETNXTEXTSIZE ) ;

      dmsExtent extentHead ;
      ossMemset( (void*)&extentHead, 0, sizeof(dmsExtent) ) ;
      INT64 read = 0 ;

      rc = ossReadN( &_file, sizeof(dmsExtent), (CHAR*)&extentHead, read ) ;
      if ( rc )
      {
         if ( SDB_EOF != rc )
         {
            PD_LOG( PDERROR, "Failed to read extent header from file, rc: %d",
                    rc ) ;
         }
         goto error ;
      }

      if ( DMS_EXTENT_EYECATCHER0 != extentHead._eyeCatcher[0] ||
           DMS_EXTENT_EYECATCHER1 != extentHead._eyeCatcher[1] )
      {
         PD_LOG ( PDERROR, "Invalid eye catcher" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      size = extentHead._blockSize * _pageSize ;
      rc = ossSeek ( &_file, (INT64)( 0 - sizeof(dmsExtent) ),
                     OSS_SEEK_CUR ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to seek back %d bytes offset, rc = %d",
                  sizeof(dmsExtent) ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSROUNIT_GETNXTEXTSIZE, rc );
      return rc ;
   error :
      size = 0 ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSROUNIT_EXPEXT, "_dmsReorgUnit::exportExtent" )
   INT32 _dmsReorgUnit::exportExtent( CHAR *pBuffer )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSROUNIT_EXPEXT );

      dmsExtent *extent = (dmsExtent*)pBuffer ;
      ossMemset ( pBuffer, 0, sizeof(dmsExtent) ) ;
      INT64 read = 0 ;
      INT64 restSize = 0 ;

      rc = ossReadN( &_file, sizeof(dmsExtent), pBuffer, read ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to read extent header, rc: %d", rc ) ;
         goto error ;
      }

      if ( DMS_EXTENT_EYECATCHER0 != extent->_eyeCatcher[0] ||
           DMS_EXTENT_EYECATCHER1 != extent->_eyeCatcher[1] )
      {
         PD_LOG ( PDERROR, "Invalid eye catcher" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      restSize = extent->_blockSize * _pageSize - sizeof(dmsExtent) ;
      rc = ossReadN( &_file, restSize, &pBuffer[sizeof(dmsExtent)], read ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to read extent body from file, rc: %d",
                 rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSROUNIT_EXPEXT, rc );
      return rc ;
   error :
      goto done ;
   }

}

