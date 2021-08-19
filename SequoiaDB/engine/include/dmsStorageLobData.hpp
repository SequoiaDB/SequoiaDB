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

   Source File Name = dmsStorageLobData.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          17/07/2014  YW Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_STORAGELOBDATA_HPP_
#define DMS_STORAGELOBDATA_HPP_

#include "dmsLobDef.hpp"
#include "ossIO.hpp"
#include "dmsStorageBase.hpp"
#include "pmdEDU.hpp"
#include "utilCache.hpp"

namespace engine
{
   #define DMS_LOBD_FLAG_NULL       0x00000000
   #define DMS_LOBD_FLAG_DIRECT     0x00000001
   #define DMS_LOBD_FLAG_SPARSE     0x00000002

   #define DMS_LOBD_EYECATCHER            "SDBLOBD"
   #define DMS_LOBD_EYECATCHER_LEN        8

   /*
      _dmsStorageLobData define
   */
   class _dmsStorageLobData : public utilCachFileBase
   {
   public:
      _dmsStorageLobData( const CHAR *fileName,
                          BOOLEAN enableSparse,
                          BOOLEAN useDirectIO ) ;
      virtual ~_dmsStorageLobData() ;

      void     enableSparse( BOOLEAN sparse ) ;

   /// Base class functions
   public:
      virtual const CHAR*     getFileName() const ;

      virtual INT32  prepareWrite( INT32 pageID,
                                   const CHAR *pData,
                                   UINT32 len,
                                   UINT32 offset,
                                   IExecutor *cb ) ;

      virtual INT32  write( INT32 pageID, const CHAR *pData,
                            UINT32 len, UINT32 offset,
                            UINT32 newestMask,
                            IExecutor *cb ) ;

      virtual INT32  prepareRead( INT32 pageID,
                                  CHAR *pData,
                                  UINT32 len,
                                  UINT32 offset,
                                  IExecutor *cb ) ;

      virtual INT32  read( INT32 pageID, CHAR *pData,
                           UINT32 len, UINT32 offset,
                           UINT32 &readLen,
                           IExecutor *cb ) ;

      virtual INT64 pageID2Offset( INT32 pageID,
                                   UINT32 pageOffset = 0 ) const
      {
         return getSeek( pageID, pageOffset ) ;
      }

      virtual INT32  offset2PageID( INT64 offset,
                                    UINT32 *pageOffset = NULL ) const
      {
         return getPageID( offset, pageOffset ) ;
      }

      virtual INT32  writeRaw( INT64 offset, const CHAR *pData,
                               UINT32 len, IExecutor *cb,
                               BOOLEAN isAligned,
                               UINT32 newestMask = UTIL_WRITE_NEWEST_BOTH ) ;

      virtual INT32  readRaw( INT64 offset, UINT32 len,
                              CHAR *buf, UINT32 &readLen,
                              IExecutor *cb,
                              BOOLEAN isAligned ) ;

   public:
      OSS_INLINE INT64 getFileSz() const
      {
         return _fileSz ;
      }

      OSS_INLINE INT64 getDataSz() const
      {
         return getFileSz() - sizeof( _dmsStorageUnitHeader ) ;
      }

      OSS_INLINE BOOLEAN isDirectIO() const
      {
         return OSS_BIT_TEST( _flags, DMS_LOBD_FLAG_DIRECT ) ? TRUE : FALSE ;
      }

      OSS_INLINE BOOLEAN isEnableSparse() const
      {
         return OSS_BIT_TEST( _flags, DMS_LOBD_FLAG_SPARSE ) ? TRUE : FALSE ;
      }

      OSS_INLINE UINT32 pageSize() const { return _pageSz ; }
      OSS_INLINE UINT32 pageSizeSquareRoot() const { return _logarithmic ; }

      OSS_INLINE UINT32 getSegmentSize() const { return DMS_SEGMENT_SZ ; }
      OSS_INLINE UINT32 segmentPages() const { return _segmentPages ; }
      OSS_INLINE UINT32 segmentPagesSquareRoot() const { return _segmentPagesSquare ; }

      INT32 open( const CHAR *path,
                  BOOLEAN createNew,
                  UINT32 totalDataPages,
                  const dmsStorageInfo &info,
                  _pmdEDUCB *cb ) ;

      INT32 rename( const CHAR *csName,
                    const CHAR *suFileName,
                    _pmdEDUCB *cb ) ;

      BOOLEAN isOpened() const ;

      INT32 close() ;

      INT32 remove() ;

      INT32 extend( INT64 len ) ;

      INT32 flush() ;

   private:
      INT32 _initFileHeader( const dmsStorageInfo &info,
                             _pmdEDUCB *cb ) ;

      INT32 _validateFile( const dmsStorageInfo &info,
                           _pmdEDUCB *cb ) ;

      INT32 _getFileHeader( _dmsStorageUnitHeader &header,
                            _pmdEDUCB *cb ) ;

      INT32 _writeFileHeader( const _dmsStorageUnitHeader &header,
                              _pmdEDUCB *cb ) ;

      OSS_INLINE SINT64 getSeek( DMS_LOB_PAGEID page,
                                 UINT32 offset ) const
      {
         if ( page < 0 )
         {
            return (INT64)offset ;
         }
         else
         {
            INT64 seek = page ;
            return ( seek << _logarithmic ) +
                   sizeof( _dmsStorageUnitHeader ) + offset ;
         }
      }

      OSS_INLINE INT32  getPageID( INT64 offset,
                                   UINT32 *pageOffset ) const
      {
         INT32 pageID = -1 ;
         if ( offset < (INT64)sizeof( _dmsStorageUnitHeader ) )
         {
            if ( pageOffset )
            {
               *pageOffset = ( UINT32 )offset ;
            }
         }
         else
         {
            offset -= sizeof( _dmsStorageUnitHeader ) ;
            pageID = offset >> _logarithmic ;
            if ( pageOffset )
            {
               *pageOffset = offset & ( _pageSz - 1 ) ;
            }
         }
         return pageID ;
      }

      INT32 _extend( INT64 len ) ;

      INT32 _postOpen( INT32 cause ) ;

   private:
      std::string       _fileName ;
      CHAR              _fullPath[ OSS_MAX_PATHSIZE + 1 ] ;
      OSSFILE           _file ;
      INT64             _fileSz ;
      UINT32            _pageSz ;
      UINT32            _logarithmic ;
      UINT32            _flags ; 
      UINT32            _segmentPages ;
      UINT32            _segmentPagesSquare ;

   } ;
   typedef class _dmsStorageLobData dmsStorageLobData ;
}

#endif // DMS_STORAGELOBDATA_HPP_

