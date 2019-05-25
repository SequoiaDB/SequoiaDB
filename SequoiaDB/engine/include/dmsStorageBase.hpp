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

   Source File Name = dmsStorageBase.hpp

   Descriptive Name = Data Management Service Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/08/2013  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSSTORAGE_BASE_HPP_
#define DMSSTORAGE_BASE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossMmap.hpp"
#include "dms.hpp"
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "../bson/bson.h"
#include "../bson/bsonobj.h"
#include "../bson/oid.h"
#include "dmsSMEMgr.hpp"
#include "dmsLobDef.hpp"
#include "pmdEnv.hpp"
#include "sdbIPersistence.hpp"
#include "dmsExtDataHandler.hpp"

#include <string>

using namespace std ;
using namespace bson ;

namespace engine
{

   #define DMS_HEADER_EYECATCHER_LEN         (8)
   #define DMS_SU_NAME_SZ                    DMS_COLLECTION_SPACE_NAME_SZ

#pragma pack(4)
   /*
      _dmsStorageInfo defined
   */
   struct _dmsStorageInfo
   {
      UINT32      _pageSize ;
      CHAR        _suName [ DMS_SU_NAME_SZ + 1 ] ; // storage unit file name is
      UINT32      _sequence ;
      UINT64      _secretValue ;
      UINT32       _lobdPageSize ;

      UINT32      _overflowRatio ;
      UINT32      _extentThreshold ;

      BOOLEAN     _enableSparse ;
      BOOLEAN     _directIO ;
      UINT32      _cacheMergeSize ;
      UINT32      _pageAllocTimeout ;

      BOOLEAN     _dataIsOK ;
      UINT64      _curLSNOnStart ;

      DMS_STORAGE_TYPE _type ;
      IDmsExtDataHandler *_extDataHandler ;

      _dmsStorageInfo ()
      {
         _pageSize      = DMS_PAGE_SIZE_DFT ;
         ossMemset( _suName, 0, sizeof( _suName ) ) ;
         _sequence      = 0 ;
         _secretValue   = 0 ;
         _lobdPageSize  = DMS_DO_NOT_CREATE_LOB ;

         _overflowRatio = 0 ;
         _extentThreshold = 0 ;
         _enableSparse = FALSE ;
         _directIO = FALSE ;
         _cacheMergeSize = 0 ;
         _pageAllocTimeout = 0 ;

         _dataIsOK       = FALSE ;
         _curLSNOnStart  = ~0 ;
         _type = DMS_STORAGE_NORMAL ;
         _extDataHandler = NULL ;
      }
   };
   typedef _dmsStorageInfo dmsStorageInfo ;

   /*
      Storage Unit Header : 65536(64K)
   */
   struct _dmsStorageUnitHeader : public SDBObject
   {
      CHAR   _eyeCatcher[DMS_HEADER_EYECATCHER_LEN] ;
      UINT32 _version ;
      UINT32 _pageSize ;                                 // size of byte
      UINT32 _storageUnitSize ;                          // all file pages
      CHAR   _name [ DMS_SU_NAME_SZ+1 ] ;                // storage unit name
      UINT32 _sequence ;                                 // storage unit seq
      UINT32 _numMB ;                                    // Number of MB
      UINT32 _MBHWM ;
      UINT32 _pageNum ;                                  // current page number
      UINT64 _secretValue ;                              // with the index
      UINT32 _lobdPageSize ;                             // lobd page size
      UINT32 _createLobs ;                               // create lob files
      UINT32 _commitFlag ;                               // commit flag
      UINT64 _commitLsn ;                                // commit LSN
      UINT64 _commitTime ;                               // commit timestamp
      CHAR   _pad [ 65336 ] ;

      _dmsStorageUnitHeader()
      {
         reset() ;
      }

      void reset()
      {
         SDB_ASSERT( DMS_PAGE_SIZE_MAX == sizeof( _dmsStorageUnitHeader ),
                     "_dmsStorageUnitHeader size must be 64K" ) ;
         ossMemset( this, 0, DMS_PAGE_SIZE_MAX ) ;
         _commitLsn = ~0 ;
      }
   } ;
   typedef _dmsStorageUnitHeader dmsStorageUnitHeader ;
   #define DMS_HEADER_SZ   sizeof(dmsStorageUnitHeader)

   #define DMS_SME_LEN                 (DMS_MAX_PG/8)
   #define DMS_SME_FREE                 0
   #define DMS_SME_ALLOCATED            1

   /* Space Management Extent, 1 bit for 1 page */
   struct _dmsSpaceManagementExtent : public SDBObject
   {
      CHAR _smeMask [ DMS_SME_LEN ] ;

      _dmsSpaceManagementExtent()
      {
         SDB_ASSERT( DMS_SME_LEN == sizeof( _dmsSpaceManagementExtent ),
                     "SME size error" ) ;
         ossMemset( _smeMask, DMS_SME_FREE, sizeof( _smeMask ) ) ;
      }
      CHAR getBitMask( UINT32 bitNum ) const
      {
         SDB_ASSERT( bitNum < DMS_MAX_PG, "Invalid bitNum" ) ;
         return (_smeMask[bitNum >> 3] >> (7 - (bitNum & 7))) & 1 ;
      }
      void freeBitMask( UINT32 bitNum )
      {
         SDB_ASSERT( bitNum < DMS_MAX_PG, "Invalid bitNum" ) ;
         _smeMask[bitNum >> 3] &= ~( 1 << (7 - (bitNum & 7))) ;
      }
      void setBitMask( UINT32 bitNum )
      {
         SDB_ASSERT( bitNum < DMS_MAX_PG, "Invalid bitNum" ) ;
         _smeMask[bitNum >> 3] |= ( 1 << (7 - (bitNum & 7))) ;
      }
   } ;
   typedef _dmsSpaceManagementExtent dmsSpaceManagementExtent ;
   #define DMS_SME_SZ  sizeof(dmsSpaceManagementExtent)

#pragma pack()

   void smeMask2String( CHAR state, CHAR *pBuffer, INT32 buffSize ) ;

   /*
      DMS_CHG_STEP define
   */
   enum DMS_CHG_STEP
   {
      DMS_CHG_BEFORE    = 1,
      DMS_CHG_AFTER
   } ;

   /*
      _dmsDirtyList define
   */
   class _dmsDirtyList : public SDBObject
   {
      public:
         _dmsDirtyList() ;
         ~_dmsDirtyList() ;

         INT32    init( UINT32 capacity ) ;
         void     destory() ;
         void     setSize( UINT32 size ) ;

         void     setDirty( UINT32 pos )
         {
            SDB_ASSERT( pos < _size, "Invalid pos" ) ;
            _pData[pos >> 3] |= ( 1 << (7 - (pos & 7))) ;
            _dirtyBegin.swapLesserThan( pos ) ;
            _dirtyEnd.swapGreaterThan( pos ) ;
         }

         void     cleanDirty( UINT32 pos )
         {
            SDB_ASSERT( pos < _size, "Invalid pos" ) ;
            _pData[pos >> 3] &= ~( 1 << (7 - (pos & 7))) ;
         }

         BOOLEAN  isDirty( UINT32 pos ) const
         {
            SDB_ASSERT( pos < _size, "Invalid pos" ) ;
            if ( _fullDirty )
            {
               return TRUE ;
            }
            return ( (_pData[pos >> 3] >> (7 - (pos & 7))) & 1 ) ? TRUE : FALSE ;
         }

         void     setFullDirty() { _fullDirty = TRUE ; }
         BOOLEAN  isFullDirty() const { return _fullDirty ; }

         INT32    nextDirtyPos( UINT32 &fromPos ) const ;
         void     cleanAll() ;
         UINT32   dirtyNumber() const ;
         UINT32   dirtyGap() const ;

      private:
         CHAR     *_pData ;
         UINT32   _capacity ;
         UINT32   _size ;
         BOOLEAN  _fullDirty ;

         ossAtomic32 _dirtyBegin ;
         ossAtomic32 _dirtyEnd ;
   } ;
   typedef _dmsDirtyList dmsDirtyList ;

   class _dmsStorageBase ;

   #define DMS_RW_ATTR_DIRTY                 0x00000001
   #define DMS_RW_ATTR_NOTHROW               0x00000002
   /*
      _dmsExtRW define
   */
   class _dmsExtRW
   {
      friend class _dmsStorageBase ;

      public:
         _dmsExtRW() ;
         ~_dmsExtRW() ;

         BOOLEAN        isEmpty() const ;
         _dmsExtRW      derive( INT32 extentID ) ;

         void           setNothrow( BOOLEAN nothrow ) ;
         BOOLEAN        isNothrow() const ;

         void           setCollectionID( INT32 id ) { _collectionID = id ; }

         INT32          getExtentID() const { return _extentID ; }
         INT32          getCollectionID() const { return _collectionID ; }

         /*
            readPtr and writePtr will throw pdGeneralException
            with error. Use pdGetLastError() can get the error number

            If you set nothrow attribute, the both functions will return
            NULL instead of throw pdGeneralException with error.
         */
         const CHAR*    readPtr( UINT32 offset, UINT32 len ) ;
         CHAR*          writePtr( UINT32 offset, UINT32 len ) ;

         template< typename T >
         const T*       readPtr( UINT32 offset = 0,
                                 UINT32 len = sizeof(T) )
         {
            return ( const T* )readPtr( offset, len ) ;
         }

         template< typename T >
         T*             writePtr( UINT32 offset = 0,
                                  UINT32 len = sizeof(T) )
         {
            return ( T* )writePtr( offset, len ) ;
         }

         BOOLEAN        isDirty() const ;

         std::string    toString() const ;

      protected:
         void           _markDirty() ;

      private:
         INT32                _extentID ;
         INT32                _collectionID ;
         UINT32               _attr ;
         ossValuePtr          _ptr ;
         _dmsStorageBase      *_pBase ;
   } ;
   typedef _dmsExtRW dmsExtRW ;

   /*
      _dmsContext define
   */
   class _dmsContext : public SDBObject
   {
      public:
         _dmsContext () {}
         virtual ~_dmsContext () {}

      public:
         virtual string toString () const = 0 ;
         virtual INT32  pause () = 0 ;
         virtual INT32  resume () = 0 ;

         virtual UINT16 mbID() const = 0 ;

   };
   typedef _dmsContext  dmsContext ;

   #define DMS_SU_FILENAME_SZ       ( DMS_SU_NAME_SZ + 15 )
   #define DMS_HEADER_OFFSET        ( 0 )
   #define DMS_SME_OFFSET           ( DMS_HEADER_OFFSET + DMS_HEADER_SZ )

   /*
      Storage Unit Base
   */
   class _dmsStorageBase : public _ossMmapFile, public IDataSyncBase
   {
      friend class _dmsExtendSegmentJob ;

      public:
         _dmsStorageBase( const CHAR *pSuFileName,
                          dmsStorageInfo *pInfo ) ;
         virtual ~_dmsStorageBase() ;

      public:
         virtual BOOLEAN      isClosed() const ;
         virtual BOOLEAN      canSync( BOOLEAN &force ) const ;

         virtual INT32        sync( BOOLEAN force,
                                    BOOLEAN sync,
                                    IExecutor* cb ) ;

         virtual void         lock() ;
         virtual void         unlock() ;

         void                 setSyncConfig( UINT32 syncInterval,
                                             UINT32 syncRecordNum,
                                             UINT32 syncDirtyRatio ) ;
         void                 setSyncDeep( BOOLEAN syncDeep ) ;
         void                 setSyncNoWriteTime( UINT32 millsec ) ;

         BOOLEAN              isSyncDeep() const ;
         UINT32               getSyncInterval() const ;
         UINT32               getSyncRecordNum() const ;
         UINT32               getSyncDirtyRatio() const ;
         UINT32               getSyncNoWriteTime() const ;

         UINT64               getCommitLSN() const ;
         UINT32               getCommitFlag() const ;
         UINT64               getCommitTime() const ;

         void                 restoreForCrash() ;
         BOOLEAN              isCrashed() const ;

         void                 enableSync( BOOLEAN enable ) ;

         IDataSyncManager*    getSyncMgr() { return _pSyncMgr ; }

      public:
         const CHAR*    getSuFileName() const ;
         const CHAR*    getSuName() const ;
         const dmsStorageUnitHeader *getHeader() { return _dmsHeader ; }
         const dmsSpaceManagementExtent *getSME () { return _dmsSME ; }
         dmsSMEMgr *getSMEMgr () { return &_smeMgr ; }

         OSS_INLINE UINT64  dataSize () const ;
         OSS_INLINE UINT64  fileSize () const ;

         OSS_INLINE UINT32  pageSize () const ;
         OSS_INLINE UINT32  pageSizeSquareRoot () const ;
         OSS_INLINE UINT32  getLobdPageSize() const ;
         OSS_INLINE UINT32  segmentPages () const ;
         OSS_INLINE UINT32  segmentPagesSquareRoot () const ;
         OSS_INLINE UINT32  maxSegmentNum() const ;
         OSS_INLINE UINT32  pageNum () const ;
         OSS_INLINE UINT32  freePageNum () const ;
         OSS_INLINE INT32   maxSegID () const ;
         OSS_INLINE UINT32  dataStartSegID () const ;
         OSS_INLINE BOOLEAN isTempSU () const { return _isTempSU ; }
         OSS_INLINE BOOLEAN isBlockScanSupport() const
         {
            return _blockScanSupport ;
         }

         OSS_INLINE UINT32  extent2Segment( dmsExtentID extentID,
                                            UINT32 *pSegOffset = NULL ) ;
         OSS_INLINE dmsExtentID segment2Extent( UINT32 segID,
                                                UINT32 segOffset = 0 ) ;

         OSS_INLINE dmsExtRW    extent2RW( INT32 extentID,
                                           INT32 collectionID = -1 ) ;
         OSS_INLINE dmsExtentID rw2extentID( const dmsExtRW &rw ) ;

         OSS_INLINE const ossValuePtr beginFixedAddr( INT32 extentID,
                                                      UINT32 pageNum ) ;
         OSS_INLINE void        endFixedAddr( const ossValuePtr ptr ) ;

         OSS_INLINE void        markAllDirty( DMS_CHG_STEP step ) ;
         OSS_INLINE void        markDirty( INT32 collectionID,
                                           INT32 extentID,
                                           DMS_CHG_STEP step ) ;

         OSS_INLINE DMS_STORAGE_TYPE getStorageType()
         {
            return _pStorageInfo->_type ;
         }

      private:
         /*
            Make these function internal
         */
         OSS_INLINE ossValuePtr extentAddr( INT32 extentID ) ;
         OSS_INLINE dmsExtentID extentID( ossValuePtr extendAddr ) ;

      public:
         INT32 openStorage ( const CHAR *pPath,
                             IDataSyncManager *pSyncMgr,
                             BOOLEAN createNew = TRUE ) ;
         void  closeStorage () ;
         INT32 removeStorage() ;

         INT32 renameStorage( const CHAR *csName,
                              const CHAR *suFileName ) ;

         INT32 flushHeader( BOOLEAN sync = FALSE ) ;
         INT32 flushSME( BOOLEAN sync = FALSE ) ;
         INT32 flushMeta( BOOLEAN sync = FALSE,
                          UINT32 *pExceptID = NULL,
                          UINT32 num = 0 ) ;
         INT32 flushPages( SINT32 pageID, UINT16 pageNum,
                           BOOLEAN sync = FALSE ) ;
         INT32 flushSegment( UINT32 segmentID, BOOLEAN sync = FALSE ) ;
         INT32 flushAll( BOOLEAN sync = FALSE ) ;
         INT32 flushDirtySegments( UINT32 *pNum = NULL,
                                   BOOLEAN force = TRUE,
                                   BOOLEAN sync = TRUE ) ;

         virtual void  syncMemToMmap () {}
         virtual BOOLEAN isOpened() const { return ossMmapFile::_opened ; }

      private:
         virtual const CHAR*  _getEyeCatcher() const = 0 ;
         virtual UINT64 _dataOffset()  = 0 ;
         virtual UINT32 _curVersion() const = 0 ;
         virtual INT32  _checkVersion( dmsStorageUnitHeader *pHeader ) = 0 ;
         virtual INT32  _onCreate( OSSFILE *file, UINT64 curOffSet ) = 0 ;
         virtual INT32  _onMapMeta( UINT64 curOffSet ) = 0 ;
         virtual void   _onClosed() {}
         virtual INT32  _onOpened() { return SDB_OK ; }
         virtual UINT32 _extendThreshold() const ;
         virtual UINT32 _getSegmentSize() const ;
         virtual void   _initHeaderPageSize( dmsStorageUnitHeader *pHeader,
                                             dmsStorageInfo *pInfo ) ;
         virtual INT32  _checkPageSize( dmsStorageUnitHeader *pHeader ) ;

         /*
            For Persistence
         */
         virtual INT32  _onFlushDirty( BOOLEAN force, BOOLEAN sync )
         {
            return SDB_OK ;
         }

         virtual INT32  _onMarkHeaderValid( UINT64 &lastLSN,
                                            BOOLEAN sync,
                                            UINT64 lastTime )
         {
            return SDB_OK ;
         }

         virtual INT32  _onMarkHeaderInvalid( INT32 collectionID )
         {
            return SDB_OK ;
         }

         virtual void _onAllocSpaceReady( dmsContext *context, BOOLEAN &doit )
         {
            doit = TRUE ;
         }

         virtual UINT64 _getOldestWriteTick() const { return ~0 ; }

         virtual void   _onRestore() {}

      protected:
         virtual INT32 _extendSegments( UINT32 numSeg ) ;

      protected:
         INT32    _findFreeSpace ( UINT16 numPages, SINT32 &foundPage,
                                   dmsContext *context ) ;
         INT32    _releaseSpace ( SINT32 pageStart, UINT16 numPages ) ;

         UINT32   _totalFreeSpace() ;

         INT32    _writeFile( OSSFILE *file, const CHAR *pData,
                              INT64 dataLen ) ;

         INT32    _markHeaderValid( BOOLEAN sync,
                                    BOOLEAN force,
                                    BOOLEAN hasFlushedData = TRUE,
                                    UINT64 lastTime = 0 ) ;

         void     _markHeaderInvalid( INT32 collectionID,
                                      BOOLEAN isAll ) ;

         void     _incWriteRecord() ;

         void     _disableBlockScan() ;

      private:
         INT32    _initializeStorageUnit () ;
         void     _initHeader ( dmsStorageUnitHeader *pHeader ) ;
         INT32    _validateHeader( dmsStorageUnitHeader *pHeader ) ;
         INT32    _preExtendSegment() ;
         INT32    _postOpen( INT32 cause ) ;

      protected:
         dmsStorageUnitHeader          *_dmsHeader ;     // 64KB
         dmsSpaceManagementExtent      *_dmsSME ;        // 8MB
         CHAR                          _suFileName[ DMS_SU_FILENAME_SZ + 1 ] ;

         dmsStorageInfo                *_pStorageInfo ;
         UINT32                        _pageSize ;    // cache, not use header
         UINT32                        _lobPageSize ; // cache, not use header

      private:
         dmsDirtyList                  _dirtyList ;
         IDataSyncManager              *_pSyncMgr ;
         ossSpinXLatch                 _persistLatch ;
         ossSpinXLatch                 _commitLatch ;
         BOOLEAN                       _isClosed ;
         volatile UINT32               _commitFlag ;
         BOOLEAN                       _isCrash ;
         BOOLEAN                       _forceSync ;

         UINT32                        _syncInterval ;
         UINT32                        _syncRecordNum ;
         UINT32                        _syncDirtyRatio ; /// not use, reserved
         UINT32                        _syncNoWriteTime ;
         BOOLEAN                       _syncDeep ;

         volatile UINT64               _lastWriteTick ;
         volatile UINT32               _writeReordNum ;
         UINT64                        _lastSyncTime ;

         BOOLEAN                       _syncEnable ;

      private:
         ossSpinSLatch                 _segmentLatch ;
         dmsSMEMgr                     _smeMgr ;
         UINT32                        _dataSegID ;
         UINT32                        _pageNum ;
         INT32                         _maxSegID ;
         UINT32                        _segmentPages ;
         UINT32                        _segmentPagesSquare ;
         UINT32                        _pageSizeSquare ;
         CHAR                          _fullPathName[ OSS_MAX_PATHSIZE + 1 ] ;
         BOOLEAN                       _isTempSU ;
         BOOLEAN                       _blockScanSupport ;
   } ;
   typedef _dmsStorageBase dmsStorageBase ;

   /*
      _dmsStorageBase OSS_INLINE functions :
   */
   OSS_INLINE UINT32 _dmsStorageBase::pageSize () const
   {
      return _pageSize ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::pageSizeSquareRoot () const
   {
      return _pageSizeSquare ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::getLobdPageSize() const
   {
      return _lobPageSize ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::segmentPages () const
   {
      return _segmentPages ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::segmentPagesSquareRoot () const
   {
      return _segmentPagesSquare ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::maxSegmentNum() const
   {
      return DMS_MAX_PG >> _segmentPagesSquare ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::pageNum () const
   {
      return _pageNum ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::freePageNum () const
   {
      return _smeMgr.totalFree() ;
   }
   OSS_INLINE INT32 _dmsStorageBase::maxSegID () const
   {
      return _maxSegID ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::dataStartSegID () const
   {
      return _dataSegID ;
   }
   OSS_INLINE UINT64 _dmsStorageBase::dataSize () const
   {
      return (UINT64)_pageNum << _pageSizeSquare ;
   }
   OSS_INLINE UINT64 _dmsStorageBase::fileSize () const
   {
      if ( _dmsHeader )
      {
         return (UINT64)_dmsHeader->_storageUnitSize << _pageSizeSquare ;
      }
      return 0 ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::extent2Segment( dmsExtentID extentID,
                                                      UINT32 * pSegOffset )
   {
      if ( pSegOffset )
      {
         *pSegOffset = extentID & (( 1 << _segmentPagesSquare ) - 1 ) ;
      }
      return ( extentID >> _segmentPagesSquare ) + _dataSegID ;
   }
   OSS_INLINE dmsExtentID _dmsStorageBase::segment2Extent( UINT32 segID,
                                                           UINT32 segOffset )
   {
      if ( segID < _dataSegID )
      {
         return DMS_INVALID_EXTENT ;
      }
      return (( segID - _dataSegID ) << _segmentPagesSquare ) + segOffset ;
   }
   OSS_INLINE ossValuePtr _dmsStorageBase::extentAddr( INT32 extentID )
   {
      if ( DMS_INVALID_EXTENT == extentID )
      {
         return 0 ;
      }
      UINT32 segOffset = 0 ;
      UINT32 segID = extent2Segment( extentID, &segOffset ) ;
      if ( segID > (UINT32)_maxSegID )
      {
         return 0 ;
      }
      return getSegmentInfo( segID ) +
             (ossValuePtr)( segOffset << _pageSizeSquare ) ;
   }
   OSS_INLINE dmsExtentID _dmsStorageBase::extentID( ossValuePtr extendAddr )
   {
      if ( 0 == extendAddr || _maxSegID < 0 )
      {
         return DMS_INVALID_EXTENT ;
      }
      INT32 segID = 0 ;
      UINT32 segOffset = 0 ;
      ossValuePtr tmpPtr = 0 ;
      UINT32 tmpLength = 0 ;
      while ( segID <= _maxSegID )
      {
         tmpPtr = getSegmentInfo( segID, &tmpLength ) ;
         if ( tmpPtr >= extendAddr &&
              extendAddr < tmpPtr + (ossValuePtr)tmpLength )
         {
            segOffset = (UINT32)((extendAddr - tmpPtr) >>
                                  _pageSizeSquare) ;
            break ;
         }
         ++segID ;
      }
      if ( segID > _maxSegID )
      {
         return DMS_INVALID_EXTENT ;
      }
      return segment2Extent( (UINT32)segID, segOffset ) ;
   }
   OSS_INLINE dmsExtRW _dmsStorageBase::extent2RW( INT32 extentID,
                                                   INT32 collectionID )
   {
      dmsExtRW rw ;
      rw._pBase = this ;
      rw._extentID = extentID ;
      rw._collectionID = collectionID ;
      rw._ptr = extentAddr( extentID ) ;
      return rw ;
   }
   OSS_INLINE dmsExtentID _dmsStorageBase::rw2extentID( const dmsExtRW &rw )
   {
      return extentID( rw._ptr ) ;
   }
   OSS_INLINE const ossValuePtr _dmsStorageBase::beginFixedAddr( INT32 extentID,
                                                                 UINT32 pageNum )
   {
      return extentAddr( extentID ) ;
   }
   OSS_INLINE void _dmsStorageBase::endFixedAddr( const ossValuePtr ptr )
   {
   }
   OSS_INLINE void _dmsStorageBase::markAllDirty( DMS_CHG_STEP step )
   {
      _markHeaderInvalid( -1, TRUE ) ;
      if ( DMS_CHG_BEFORE == step )
      {
         return ;
      }
      _lastWriteTick = pmdGetDBTick() ;
      _dirtyList.setFullDirty() ;
      if ( _pSyncMgr && _syncRecordNum > 0 &&
           _writeReordNum >= _syncRecordNum )
      {
         _pSyncMgr->notifyChange() ;
      }
   }
   OSS_INLINE void _dmsStorageBase::markDirty( INT32 collectionID,
                                               INT32 extentID,
                                               DMS_CHG_STEP step )
   {
      UINT32 segID = extent2Segment( extentID, NULL ) ;
      if ( (INT32)segID <= _maxSegID )
      {
         _markHeaderInvalid( collectionID, FALSE ) ;
         if ( DMS_CHG_BEFORE == step )
         {
            return ;
         }
         _lastWriteTick = pmdGetDBTick() ;
         _dirtyList.setDirty( segID - _dataSegID ) ;

         if ( _pSyncMgr && _syncRecordNum > 0 &&
              _writeReordNum >= _syncRecordNum )
         {
            _pSyncMgr->notifyChange() ;
         }
      }
   }

   /*
      DMS Other define
   */
   #define DMS_MON_OP_COUNT_INC( _pMonAppCB_, op, delta )  \
   {                                                       \
      if ( NULL != _pMonAppCB_ )                           \
      {                                                    \
         _pMonAppCB_->monOperationCountInc( op, delta ) ;  \
      }                                                    \
   }

   #define DMS_MON_OP_TIME_INC( _pMonAppCB_, op, delta )   \
   {                                                       \
      if ( NULL != _pMonAppCB_ )                           \
      {                                                    \
         _pMonAppCB_->monOperationTimeInc( op, delta ) ;   \
      }                                                    \
   }

   #define DMS_MON_CONTEXT_COUNT_INC( _monContextCB_, op, delta ) \
   {                                                               \
      if ( NULL != _monContextCB_ )                                \
      {                                                            \
         _monContextCB_->monOperationCountInc ( op, delta ) ;      \
      }                                                            \
   }

   #define DMS_MON_CONTEXT_TIME_INC( _monContextCB_, op, delta )  \
   {                                                               \
      if ( NULL != _monContextCB_ )                                \
      {                                                            \
         _monContextCB_->monOperationTimeInc ( op, delta ) ;       \
      }                                                            \
   }

   /****************************************************************************
    * Specify the matrix for collection flag and access type, returns TRUE means
    * access is allowed, otherwise return FALSE
    * AccessType:       Query  Fetch  Insert  Update  Delete  Truncate CRT-IDX  DROP-IDX
    *  FREE                N      N       N       N       N       N       N         N
    *  NORMAL              Y      Y       Y       Y       Y       Y       Y         Y
    *  DROPPED             N      N       N       N       N       N       N         N
    *  OFFLINE REORG       N (only alloed in shadow copy phase, rebuild )
    *                             N       N       N       N       N ( only allowed in
    *  truncate phase )                                                   N         N
    *  ONLINE REORG        Y      Y       Y       Y       Y       Y       Y         Y
    *  Load                Y      Y       Y       Y       Y       N       Y         Y
    ***************************************************************************/
   BOOLEAN dmsAccessAndFlagCompatiblity ( UINT16 collectionFlag,
                                          DMS_ACCESS_TYPE accessType ) ;

}

#endif //DMSSTORAGE_BASE_HPP_

