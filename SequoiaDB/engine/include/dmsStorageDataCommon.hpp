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

   Source File Name = dmsStorageDataCommon.hpp

   Descriptive Name = Common Data Management Service Storage Unit Header

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
#ifndef DMSSTORAGE_DATACOMMON_HPP_
#define DMSSTORAGE_DATACOMMON_HPP_

#include "dmsStorageBase.hpp"
#include "dmsExtent.hpp"
#include "dpsLogWrapper.hpp"
#include "dmsCompress.hpp"
#include "dmsEventHandler.hpp"
#include "dmsExtDataHandler.hpp"

#include <map>

using namespace bson ;

namespace engine
{

#pragma pack(1)
   /*
      _IDToInsert define
   */
   class _IDToInsert : public SDBObject
   {
   public :
      CHAR _type ;
      CHAR _id[4] ; // _id + '\0'
      OID _oid ;
      _IDToInsert ()
      {
         _type = (CHAR)jstOID ;
         _id[0] = '_' ;
         _id[1] = 'i' ;
         _id[2] = 'd' ;
         _id[3] = 0 ;
         SDB_ASSERT ( sizeof ( _IDToInsert) == 17,
                      "IDToInsert should be 17 bytes" ) ;
      }
   } ;
   typedef class _IDToInsert IDToInsert ;

   /*
      _idToInsert define
   */
   class _idToInsertEle : public BSONElement
   {
   public :
      _idToInsertEle( CHAR* x ) : BSONElement((CHAR*) ( x )){}
   } ;
   typedef class _idToInsertEle idToInsertEle ;

#pragma pack()

   /*
      MB FLAG(_flag) values :
   */
   #define DMS_MB_BASE_MASK                        0x000F
   #define DMS_MB_FLAG_FREE                        0x0000
   #define DMS_MB_FLAG_USED                        0x0001
   #define DMS_MB_FLAG_DROPED                      0x0002

   #define DMS_MB_OPR_TYPE_MASK                    0x00F0
   #define DMS_MB_FLAG_OFFLINE_REORG               0x0010
   #define DMS_MB_FLAG_ONLINE_REORG                0x0020
   #define DMS_MB_FLAG_LOAD                        0x0040

   #define DMS_MB_OPR_PHASE_MASK                   0x0F00

   #define DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY   0x0100
   #define DMS_MB_FLAG_OFFLINE_REORG_TRUNCATE      0x0200
   #define DMS_MB_FLAG_OFFLINE_REORG_COPY_BACK     0x0400
   #define DMS_MB_FLAG_OFFLINE_REORG_REBUILD       0x0800

   #define DMS_MB_FLAG_LOAD_LOAD                   0x0100
   #define DMS_MB_FLAG_LOAD_BUILD                  0x0200

   #define DMS_MB_BASE_FLAG(x)                     ((x)&DMS_MB_BASE_MASK)
   #define DMS_MB_OPR_FLAG(x)                      ((x)&DMS_MB_OPR_TYPE_MASK)
   #define DMS_MB_PHASE_FLAG(x)                    ((x)&DMS_MB_OPR_PHASE_MASK)

   #define DMS_IS_MB_FREE(x)        (DMS_MB_FLAG_FREE==(x))
   #define DMS_SET_MB_FREE(x)       do {(x)=DMS_MB_FLAG_FREE ;} while(0)
   #define DMS_IS_MB_INUSE(x)       (0!=((x)&DMS_MB_FLAG_USED))
   #define DMS_SET_MB_INUSE(x)      do {(x)|=DMS_MB_FLAG_USED ;} while(0)
   #define DMS_IS_MB_DROPPED(x)     (DMS_MB_FLAG_DROPED==(x))
   #define DMS_SET_MB_DROPPED(x)    do {(x)=DMS_MB_FLAG_DROPED ;} while(0)
   #define DMS_IS_MB_NORMAL(x)      (DMS_MB_FLAG_USED==(x))
   #define DMS_SET_MB_NORMAL(x)     do {(x)=DMS_MB_FLAG_USED ;} while(0)

   #define DMS_IS_MB_OFFLINE_REORG(x)  \
      ((0!=((x)&DMS_MB_FLAG_OFFLINE_REORG))&&(DMS_IS_MB_INUSE(x)))
   #define DMS_SET_MB_OFFLINE_REORG(x) \
      do {(x)=DMS_MB_FLAG_OFFLINE_REORG|DMS_MB_FLAG_USED;} while(0)
   #define DMS_IS_MB_ONLINE_REORG(x)   \
      ((0!=((x)&DMS_MB_FLAG_ONLINE_REORG))&&(DMS_IS_MB_INUSE(x)))
   #define DMS_SET_MB_ONLINE_REORG(x)  \
      do {(x)=DMS_MB_FLAG_ONLINE_REORG|DMS_MB_FLAG_USED;} while(0)
   #define DMS_IS_MB_OFFLINE_REORG_SHADOW_COPY(x)     \
      ((0!=((x)&DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY))&&\
      (DMS_IS_MB_OFFLINE_REORG(x)))
   #define DMS_SET_MB_OFFLINE_REORG_SHADOW_COPY(x)    \
      do {(x)=DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY|DMS_MB_FLAG_OFFLINE_REORG|\
      DMS_MB_FLAG_USED;} while(0)
   #define DMS_IS_MB_OFFLINE_REORG_TRUNCATE(x)        \
      ((0!=((x)&DMS_MB_FLAG_OFFLINE_REORG_TRUNCATE))&&\
      (DMS_IS_MB_OFFLINE_REORG(x)))
   #define DMS_SET_MB_OFFLINE_REORG_TRUNCATE(x)       \
      do {(x)=DMS_MB_FLAG_OFFLINE_REORG_TRUNCATE|DMS_MB_FLAG_OFFLINE_REORG|\
      DMS_MB_FLAG_USED;} while(0)
   #define DMS_IS_MB_OFFLINE_REORG_COPY_BACK(x)       \
      ((0!=((x)&DMS_MB_FLAG_OFFLINE_REORG_COPY_BACK))&&\
      (DMS_IS_MB_OFFLINE_REORG(x)))
   #define DMS_SET_MB_OFFLINE_REORG_COPY_BACK(x)      \
      do {(x)=DMS_MB_FLAG_OFFLINE_REORG_COPY_BACK|DMS_MB_FLAG_OFFLINE_REORG|\
      DMS_MB_FLAG_USED;} while(0)
   #define DMS_IS_MB_OFFLINE_REORG_REBUILD(x)         \
      ((0!=((x)&DMS_MB_FLAG_OFFLINE_REORG_REBUILD))&&\
      (DMS_IS_MB_OFFLINE_REORG(x)))
   #define DMS_SET_MB_OFFLINE_REORG_REBUILD(x)        \
      do {(x)=DMS_MB_FLAG_OFFLINE_REORG_REBUILD|DMS_MB_FLAG_OFFLINE_REORG|\
      DMS_MB_FLAG_USED;} while(0)

   #define DMS_IS_MB_LOAD(x)                          \
      (0!=((x)&DMS_MB_FLAG_LOAD)&&(DMS_IS_MB_INUSE(x)))
   #define DMS_SET_MB_LOAD(x)                         \
      do {(x)=DMS_MB_FLAG_LOAD|DMS_MB_FLAG_USED;} while(0)
   #define DMS_IS_MB_FLAG_LOAD_LOAD(x)                \
      ((0!=((x)&DMS_MB_FLAG_LOAD_LOAD))&&(DMS_IS_MB_LOAD(x)))
   #define DMS_SET_MB_FLAG_LOAD_LOAD(x)               \
      do {(x)=DMS_MB_FLAG_LOAD_LOAD|DMS_MB_FLAG_LOAD|\
      DMS_MB_FLAG_USED;} while(0)
   #define DMS_IS_MB_FLAG_LOAD_BUILD(x)               \
      ((0!=((x)&DMS_MB_FLAG_LOAD_BUILD))&&(DMS_IS_MB_LOAD(x)))
   #define DMS_SET_MB_FLAG_LOAD_BUILD(x)              \
      do {(x)=DMS_MB_FLAG_LOAD_BUILD|DMS_MB_FLAG_LOAD|\
      DMS_MB_FLAG_USED;} while(0)

   #define DMS_MB_STATINFO_FLAG_TRUNCATED  0x00000001

   #define DMS_MB_STATINFO_IS_TRUNCATED(x) \
      (0 != ((x) & DMS_MB_STATINFO_FLAG_TRUNCATED))
   #define DMS_MB_STATINFO_SET_TRUNCATED(x) \
      do { (x) |= DMS_MB_STATINFO_FLAG_TRUNCATED ; } while (0)
   #define DMS_MB_STATINFO_CLEAR_TRUNCATED(x) \
      do { (x) &= ~DMS_MB_STATINFO_FLAG_TRUNCATED ; } while (0)

   /*
      DMS MB ATTRIBUTE DEFINE
   */
   #define DMS_MB_ATTR_COMPRESSED         0x00000001
   #define DMS_MB_ATTR_NOIDINDEX          0x00000002
   #define DMS_MB_ATTR_CAPPED             0x00000004
   #define DMS_MB_ATTR_STRICTDATAMODE     0x00000008

#pragma pack(4)
   /*
      _dmsMetadataBlock defined
   */
   struct _dmsMetadataBlock
   {
      enum deleteListType
      {
         _32 = 0,
         _64,
         _128,
         _256,
         _512,
         _1k,
         _2k,
         _4k,
         _8k,
         _16k,
         _32k,
         _64k,
         _128k,
         _256k,
         _512k,
         _1m,
         _2m,
         _4m,
         _8m,
         _16m,
         _max
      } ;

      CHAR           _collectionName [ DMS_COLLECTION_NAME_SZ+1 ] ;
      UINT16         _flag ;
      UINT16         _blockID ;
      dmsExtentID    _firstExtentID ;
      dmsExtentID    _lastExtentID ;
      UINT32         _numIndexes ;
      dmsRecordID    _deleteList [_max] ;
      dmsExtentID    _indexExtent [DMS_COLLECTION_MAX_INDEX] ;
      UINT32         _logicalID ;
      UINT32         _indexHWCount ;
      UINT32         _attributes ;
      dmsExtentID    _loadFirstExtentID ;
      dmsExtentID    _loadLastExtentID ;
      dmsExtentID    _mbExExtentID ;
      UINT64         _totalRecords ;
      UINT32         _totalDataPages ;
      UINT32         _totalIndexPages ;
      UINT64         _totalDataFreeSpace ;
      UINT64         _totalIndexFreeSpace ;
      UINT32         _totalLobPages ;

      dmsExtentID    _dictExtentID ;
      dmsExtentID    _newDictExtentID ;
      SINT32         _dictStatPageID ;
      UINT8          _dictVersion ;
      UINT8          _compressorType ;
      UINT8          _lastCompressRatio ;
      UINT8          _pad1[ 1 ] ;   // reserved
      UINT64         _totalLobs ;
      UINT64         _totalOrgDataLen ;
      UINT64         _totalDataLen ;
      CHAR           _pad2[ 16 ] ;  // reserved

      UINT32         _commitFlag ;
      UINT64         _commitLSN ;
      UINT64         _commitTime ;
      UINT32         _idxCommitFlag ;
      UINT64         _idxCommitLSN ;
      UINT64         _idxCommitTime ;
      UINT32         _lobCommitFlag ;
      UINT64         _lobCommitLSN ;
      UINT64         _lobCommitTime ;

      dmsExtentID    _mbOptExtentID ;
      CHAR           _pad [ 284 ] ;

      void reset ( const CHAR *clName = NULL,
                   UINT16 mbID = DMS_INVALID_MBID,
                   UINT32 clLID = DMS_INVALID_CLID,
                   UINT32 attr = 0,
                   UINT8 compressType = UTIL_COMPRESSOR_INVALID )
      {
         INT32 i = 0 ;
         ossMemset( _collectionName, 0, sizeof( _collectionName ) ) ;
         if ( clName )
         {
            ossStrncpy( _collectionName, clName, DMS_COLLECTION_NAME_SZ ) ;
         }
         if ( DMS_INVALID_MBID != mbID )
         {
            DMS_SET_MB_INUSE( _flag ) ;
         }
         else
         {
            DMS_SET_MB_FREE( _flag ) ;
         }
         _blockID = mbID ;
         _firstExtentID = DMS_INVALID_EXTENT ;
         _lastExtentID  = DMS_INVALID_EXTENT ;
         _numIndexes    = 0 ;
         for ( i = 0 ; i < _max ; ++i )
         {
            _deleteList[i].reset() ;
         }
         for ( i = 0 ; i < DMS_COLLECTION_MAX_INDEX ; ++i )
         {
            _indexExtent[i] = DMS_INVALID_EXTENT ;
         }
         _logicalID = clLID ;
         _indexHWCount = 0 ;
         _attributes   = attr ;
         _loadFirstExtentID = DMS_INVALID_EXTENT ;
         _loadLastExtentID  = DMS_INVALID_EXTENT ;
         _mbExExtentID      = DMS_INVALID_EXTENT ;

         _totalRecords           = 0 ;
         _totalDataPages         = 0 ;
         _totalIndexPages        = 0 ;
         _totalDataFreeSpace     = 0 ;
         _totalIndexFreeSpace    = 0 ;
         _totalLobPages          = 0 ;
         _totalLobs              = 0 ;
         _compressorType         = compressType ;
         _dictVersion            = 0 ;
         _dictExtentID           = DMS_INVALID_EXTENT ;
         _newDictExtentID        = DMS_INVALID_EXTENT ;
         _dictStatPageID         = DMS_INVALID_EXTENT ;
         _lastCompressRatio      = 100 ;

         _totalOrgDataLen        = 0 ;
         _totalDataLen           = 0 ;

         _commitFlag             = 0 ;
         _commitLSN              = ~0 ;
         _commitTime             = 0 ;
         _idxCommitFlag          = 0 ;
         _idxCommitLSN           = ~0 ;
         _idxCommitTime          = 0 ;
         _lobCommitFlag          = 0 ;
         _lobCommitLSN           = ~0 ;
         _lobCommitTime          = 0 ;

         _mbOptExtentID          = DMS_INVALID_EXTENT ;

         if ( OSS_BIT_TEST( attr, DMS_MB_ATTR_COMPRESSED ) )
         {
            _compressorType      = compressType ;
         }

         ossMemset( _pad1, 0, sizeof( _pad1 ) ) ;
         ossMemset( _pad2, 0, sizeof( _pad2 ) ) ;
         ossMemset( _pad, 0, sizeof( _pad ) ) ;
      }
   } ;
   typedef _dmsMetadataBlock  dmsMetadataBlock ;
   typedef dmsMetadataBlock   dmsMB ;
   #define DMS_MB_SIZE                 (1024)

#pragma pack()

   /*
      Type to String functions
   */
   void  mbFlag2String ( UINT16 flag, CHAR *pBuffer, INT32 bufSize ) ;
   void  mbAttr2String ( UINT32 attributes, CHAR *pBuffer, INT32 bufSize ) ;

   /*
      _metadataBlockEx define
   */
   struct _metadataBlockEx
   {
      dmsMetaExtent        _header ;
      dmsExtentID          _array[1] ;

      INT32 getFirstExtentID( UINT32 segID, dmsExtentID &extID ) const
      {
         if ( segID >= _header._segNum )
         {
            return SDB_INVALIDARG ;
         }
         UINT32 index = segID << 1 ;
         extID = _array[index] ;
         return SDB_OK ;
      }
      INT32 getLastExtentID( UINT32 segID, dmsExtentID &extID ) const
      {
         if ( segID >= _header._segNum )
         {
            return SDB_INVALIDARG ;
         }
         UINT32 index = ( segID << 1 ) + 1 ;
         extID = _array[index] ;
         return SDB_OK ;
      }
      INT32 setFirstExtentID( UINT32 segID, dmsExtentID extID )
      {
         if ( segID >= _header._segNum )
         {
            return SDB_INVALIDARG ;
         }
         UINT32 index = segID << 1 ;
         _array[index] = extID ;
         return SDB_OK ;
      }
      INT32 setLastExtentID( UINT32 segID, dmsExtentID extID )
      {
         if ( segID >= _header._segNum )
         {
            return SDB_INVALIDARG ;
         }
         UINT32 index = ( segID << 1 ) + 1 ;
         _array[index] = extID ;
         return SDB_OK ;
      }
   } ;
   typedef _metadataBlockEx   dmsMBEx ;

   #define DMS_MME_SZ               (DMS_MME_SLOTS*DMS_MB_SIZE)
   /*
      _dmsMetadataManagementExtent defined
   */
   struct _dmsMetadataManagementExtent : public SDBObject
   {
      dmsMetadataBlock  _mbList [ DMS_MME_SLOTS ] ;

      _dmsMetadataManagementExtent ()
      {
         SDB_ASSERT( DMS_MME_SZ == sizeof( _dmsMetadataManagementExtent ),
                     "MME size error" ) ;
         ossMemset( this, 0, sizeof( _dmsMetadataManagementExtent ) ) ;
      }
   } ;
   typedef _dmsMetadataManagementExtent dmsMetadataManagementExtent ;

   /*
      _dmsMBStatInfo define
   */
   struct _dmsMBStatInfo
   {
      UINT64      _totalRecords ;
      UINT32      _totalDataPages ;
      UINT32      _totalIndexPages ;
      UINT64      _totalDataFreeSpace ;
      UINT64      _totalIndexFreeSpace ;
      UINT32      _totalLobPages ;
      UINT64      _totalLobs ;
      UINT32      _uniqueIdxNum ;
      UINT32      _textIdxNum ;
      UINT8       _lastCompressRatio ;
      UINT64      _totalOrgDataLen ;
      UINT64      _totalDataLen ;
      UINT32      _startLID ;
      UINT32      _flag ;

      ossAtomic32 _commitFlag ;
      ossAtomic64 _lastLSN ;
      UINT64      _lastWriteTick ;
      BOOLEAN     _isCrash ;

      ossAtomic32 _idxCommitFlag ;
      ossAtomic64 _idxLastLSN ;
      UINT64      _idxLastWriteTick ;
      BOOLEAN     _idxIsCrash ;

      ossAtomic32 _lobCommitFlag ;
      ossAtomic64 _lobLastLSN ;
      UINT64      _lobLastWriteTick ;
      BOOLEAN     _lobIsCrash ;

      void reset()
      {
         _totalRecords           = 0 ;
         _totalDataPages         = 0 ;
         _totalIndexPages        = 0 ;
         _totalDataFreeSpace     = 0 ;
         _totalIndexFreeSpace    = 0 ;
         _totalLobPages          = 0 ;
         _totalLobs              = 0 ;
         _uniqueIdxNum           = 0 ;
         _textIdxNum             = 0 ;
         _lastCompressRatio      = 100 ;
         _totalOrgDataLen        = 0 ;
         _totalDataLen           = 0 ;
         _startLID               = DMS_INVALID_CLID ;
         _flag                   = 0 ;
         _commitFlag.init( 0 ) ;
         _lastLSN.init( ~0 ) ;
         _lastWriteTick          = 0 ;
         _isCrash                = FALSE ;
         _idxCommitFlag.init( 0 ) ;
         _idxLastLSN.init( ~0 ) ;
         _idxLastWriteTick       = 0 ;
         _idxIsCrash             = FALSE ;
         _lobCommitFlag.init( 0 ) ;
         _lobLastLSN.init( ~0 ) ;
         _lobLastWriteTick       = 0 ;
         _lobIsCrash             = FALSE ;
      }

      void updateLastLSN( UINT64 lsn, DMS_FILE_TYPE type )
      {
         if ( DMS_FILE_DATA == type )
         {
            _lastLSN.init( lsn ) ;
         }
         else if ( DMS_FILE_IDX == type )
         {
            _idxLastLSN.init( lsn ) ;
         }
         else if ( DMS_FILE_LOB == type )
         {
            _lobLastLSN.init( lsn ) ;
         }
      }
      void updateLastLSNWithComp( UINT64 lsn,
                                  DMS_FILE_TYPE type,
                                  BOOLEAN isRollback )
      {
         if ( DMS_FILE_DATA == type )
         {
            if ( !_lastLSN.compareAndSwap( DPS_INVALID_LSN_OFFSET, lsn ) )
            {
               if ( !isRollback )
               {
                  _lastLSN.swapGreaterThan( lsn ) ;
               }
               else
               {
                  _lastLSN.swapLesserThan( lsn ) ;
               }
            }
         }
         else if ( DMS_FILE_IDX == type )
         {
            if ( !_idxLastLSN.compareAndSwap( DPS_INVALID_LSN_OFFSET, lsn ) )
            {
               if ( !isRollback )
               {
                  _idxLastLSN.swapGreaterThan( lsn ) ;
               }
               else
               {
                  _idxLastLSN.swapLesserThan( lsn ) ;
               }
            }
         }
         else if ( DMS_FILE_LOB == type )
         {
            if ( !_lobLastLSN.compareAndSwap( DPS_INVALID_LSN_OFFSET, lsn ) )
            {
               if ( !isRollback )
               {
                  _lobLastLSN.swapGreaterThan( lsn ) ;
               }
               else
               {
                  _lobLastLSN.swapLesserThan( lsn ) ;
               }
            }
         }
      }

      _dmsMBStatInfo ()
      :_commitFlag( 0 ), _lastLSN( 0 ),
      _idxCommitFlag( 0 ), _idxLastLSN( 0 ),
      _lobCommitFlag( 0 ), _lobLastLSN( 0 )
      {
         reset() ;
      }
   } ;
   typedef _dmsMBStatInfo dmsMBStatInfo ;

   class _dmsStorageDataCommon ;
   /*
      _dmsMBContext define
   */
   class _dmsMBContext : public _dmsContext
   {
      friend class _dmsStorageDataCommon ;
      private:
         _dmsMBContext() ;
         virtual ~_dmsMBContext() ;
         void _reset () ;

      public:
         virtual string toString () const ;
         virtual INT32  pause () ;
         virtual INT32  resume () ;

         OSS_INLINE INT32   mbLock( INT32 lockType ) ;
         OSS_INLINE INT32   mbUnlock() ;
         OSS_INLINE BOOLEAN isMBLock( INT32 lockType ) const ;
         OSS_INLINE BOOLEAN isMBLock() const ;
         OSS_INLINE BOOLEAN canResume() const ;

         virtual     UINT16 mbID () const { return _mbID ; }
         OSS_INLINE  dmsMB* mb () { return _mb ; }
         OSS_INLINE  dmsMBStatInfo* mbStat() { return _mbStat ; }
         OSS_INLINE  UINT32 clLID () const { return _clLID ; }
         OSS_INLINE  UINT32 startLID() const { return _startLID ; }

      private:
         dmsMB             *_mb ;
         dmsMBStatInfo     *_mbStat ;
         ossSpinSLatch     *_latch ;
         UINT32            _clLID ;
         UINT32            _startLID ;
         UINT16            _mbID ;
         INT32             _mbLockType ;
         INT32             _resumeType ;
   };
   typedef _dmsMBContext   dmsMBContext ;

   /*
      _dmsMBContext OSS_INLINE functions
   */
   OSS_INLINE INT32 _dmsMBContext::mbLock( INT32 lockType )
   {
      INT32 rc = SDB_OK ;
      if ( SHARED != lockType && EXCLUSIVE != lockType )
      {
         return SDB_INVALIDARG ;
      }
      if ( _mbLockType == lockType )
      {
         return SDB_OK ;
      }
      if ( -1 != _mbLockType && SDB_OK != ( rc = pause() ) )
      {
         return rc ;
      }
      if ( !DMS_IS_MB_INUSE(_mb->_flag) )
      {
         return SDB_DMS_NOTEXIST ;
      }
      if ( _clLID != _mb->_logicalID )
      {
         if ( _startLID == _mbStat->_startLID &&
              DMS_MB_STATINFO_IS_TRUNCATED( _mbStat->_flag ) )
         {
            return SDB_DMS_TRUNCATED ;
         }
         else
         {
            return SDB_DMS_NOTEXIST ;
         }
      }
      ossLatch( _latch, (OSS_LATCH_MODE)lockType ) ;
      if ( !DMS_IS_MB_INUSE(_mb->_flag) )
      {
         ossUnlatch( _latch, (OSS_LATCH_MODE)lockType ) ;
         return SDB_DMS_NOTEXIST ;
      }
      if ( _clLID != _mb->_logicalID )
      {
         if ( _startLID == _mbStat->_startLID &&
              DMS_MB_STATINFO_IS_TRUNCATED( _mbStat->_flag ) )
         {
            ossUnlatch( _latch, (OSS_LATCH_MODE)lockType ) ;
            return SDB_DMS_TRUNCATED ;
         }
         else
         {
            ossUnlatch( _latch, (OSS_LATCH_MODE)lockType ) ;
            return SDB_DMS_NOTEXIST ;
         }
      }
      _mbLockType = lockType ;
      _resumeType = -1 ;
      return SDB_OK ;
   }
   OSS_INLINE INT32 _dmsMBContext::mbUnlock()
   {
      if ( SHARED == _mbLockType || EXCLUSIVE == _mbLockType )
      {
         ossUnlatch( _latch, (OSS_LATCH_MODE)_mbLockType ) ;
         _resumeType = _mbLockType ;
         _mbLockType = -1 ;
      }
      return SDB_OK ;
   }
   OSS_INLINE BOOLEAN _dmsMBContext::isMBLock( INT32 lockType ) const
   {
      return lockType == _mbLockType ? TRUE : FALSE ;
   }
   OSS_INLINE BOOLEAN _dmsMBContext::isMBLock() const
   {
      if ( SHARED == _mbLockType || EXCLUSIVE == _mbLockType )
      {
         return TRUE ;
      }
      return FALSE ;
   }
   OSS_INLINE BOOLEAN _dmsMBContext::canResume() const
   {
      if ( SHARED == _resumeType || EXCLUSIVE == _resumeType )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   /*
      _dmsRecordRW define
   */
   class _dmsRecordRW
   {
      friend class _dmsStorageDataCommon ;

      public:
         _dmsRecordRW() ;
         ~_dmsRecordRW() ;

         BOOLEAN           isEmpty() const ;
         _dmsRecordRW      derivePre() ;
         _dmsRecordRW      deriveNext() ;
         _dmsRecordRW      deriveOverflow() ;
         _dmsRecordRW      derive( const dmsRecordID &rid ) ;

         void              setNothrow( BOOLEAN nothrow ) ;
         BOOLEAN           isNothrow() const ;

         dmsRecordID       getRecordID() const { return _rid ; }

         /*
            When len == 0, Use the record's size
         */
         const dmsRecord*  readPtr( UINT32 len = sizeof( dmsRecord ) ) ;
         dmsRecord*        writePtr( UINT32 len = sizeof( dmsRecord ) ) ;

         template< typename T >
         const T*          readPtr( UINT32 len = sizeof(T) )
         {
            return ( const T* )readPtr( len ) ;
         }

         template< typename T >
         T*                writePtr( UINT32 len = sizeof(T)  )
         {
            return ( T* )writePtr( len ) ;
         }

         std::string       toString() const ;

      private:
         void              _doneAddr() ;

      private:
         dmsRecordID       _rid ;
         const dmsRecord   *_ptr ;
         dmsExtRW          _rw ;
         _dmsStorageDataCommon   *_pData ;
   } ;
   typedef _dmsRecordRW dmsRecordRW ;

   #define DMS_MME_OFFSET                 ( DMS_SME_OFFSET + DMS_SME_SZ )
   #define DMS_DATASU_EYECATCHER          "SDBDATA"

   #define DMS_DATASU_CUR_VERSION         3
   #define DMS_DATACAPSU_EYECATCHER       "SDBDCAP"
   #define DMS_COMPRESSION_ENABLE_VER     2
   #define DMS_CONTEXT_MAX_SIZE           (2000)
   #define DMS_RECORDS_PER_EXTENT_SQUARE  4     // value is 2^4=16
   #define DMS_RECORD_OVERFLOW_RATIO      1.2f

   /*
      DMS TRUNCATE TYPE DEFINE
   */
   enum DMS_TRUNC_TYPE
   {
      DMS_TRUNC_LOAD    = 1,
      DMS_TRUNC_ALL     = 2
   } ;

   class _dmsStorageIndex ;
   class _dmsStorageLob ;
   class _dmsStorageUnit ;
   class _pmdEDUCB ;
   class _mthModifier ;

   /*
      _dmsStorageDataCommon defined
   */
   class _dmsStorageDataCommon : public _dmsStorageBase
   {
      friend class _dmsStorageIndex ;
      friend class _dmsStorageUnit ;
      friend class _dmsStorageLob ;

      struct cmp_str
      {
         bool operator() (const char *a, const char *b)
         {
            return ossStrcmp( a, b ) < 0 ;
         }
      } ;

      typedef std::map<const CHAR*, UINT16, cmp_str>        COLNAME_MAP ;
#if defined (_WINDOWS)
      typedef COLNAME_MAP::iterator                         COLNAME_MAP_IT ;
      typedef COLNAME_MAP::const_iterator                   COLNAME_MAP_CIT ;
#else
      typedef std::map<const CHAR*, UINT16>::iterator       COLNAME_MAP_IT ;
      typedef std::map<const CHAR*, UINT16>::const_iterator COLNAME_MAP_CIT ;
#endif

      public:
         _dmsStorageDataCommon ( const CHAR *pSuFileName,
                                 dmsStorageInfo *pInfo,
                                 _IDmsEventHolder *pEventHolder ) ;
         virtual ~_dmsStorageDataCommon () ;

         virtual void  syncMemToMmap() ;

         UINT32 logicalID () const { return _logicalCSID ; }
         dmsStorageUnitID CSID () const { return _CSID ; }

         OSS_INLINE INT32  getMBContext( dmsMBContext **pContext, UINT16 mbID,
                                         UINT32 clLID, UINT32 startLID, INT32 lockType = -1 );
         OSS_INLINE INT32  getMBContext( dmsMBContext **pContext,
                                         const CHAR* pName,
                                         INT32 lockType = -1 ) ;
         OSS_INLINE INT32  checkMBContext( const CHAR *pName, UINT16 mbID ) ;
         OSS_INLINE void   releaseMBContext( dmsMBContext *&pContext ) ;

         OSS_INLINE const dmsMBStatInfo* getMBStatInfo( UINT16 mbID ) const ;
         OSS_INLINE const dmsMB* getMBInfo( UINT16 mbID ) const ;

         OSS_INLINE UINT32 getCollectionNum() ;

         OSS_INLINE dmsRecordRW record2RW( const dmsRecordID &record,
                                           UINT16 collectionID ) ;

         INT32         addExtent2Meta( dmsExtentID extID, dmsExtent *extent,
                                       dmsMBContext *context ) ;
         INT32         removeExtentFromMeta( dmsMBContext *context,
                                             dmsExtentID extID,
                                             dmsExtent *extent ) ;

         OSS_INLINE void   updateCreateLobs( UINT32 createLobs ) ;

         void regExtDataHandler( IDmsExtDataHandler *handler )
         {
            _pExtDataHandler = handler ;
         }

         void unregExtDataHandler()
         {
            _pExtDataHandler = NULL ;
         }

         IDmsExtDataHandler *getExtDataHandler()
         {
            return _pExtDataHandler ;
         }

         INT32          flushMME( BOOLEAN sync = FALSE ) ;

      public:

         INT32 addCollection ( const CHAR *pName,
                               UINT16 *collectionID,
                               UINT32 attributes = 0,
                               _pmdEDUCB * cb = NULL,
                               SDB_DPSCB *dpscb = NULL,
                               UINT16 initPages = 0,
                               BOOLEAN sysCollection = FALSE,
                               UINT8 compressionType = UTIL_COMPRESSOR_INVALID,
                               UINT32 *logicID = NULL,
                               const BSONObj *extOptions = NULL ) ;

         INT32 dropCollection ( const CHAR *pName,
                                _pmdEDUCB *cb,
                                SDB_DPSCB *dpscb,
                                BOOLEAN sysCollection = TRUE,
                                dmsMBContext *context = NULL ) ;

         INT32 truncateCollection ( const CHAR *pName,
                                    _pmdEDUCB *cb,
                                    SDB_DPSCB *dpscb,
                                    BOOLEAN sysCollection = TRUE,
                                    dmsMBContext *context = NULL,
                                    BOOLEAN needChangeCLID = TRUE,
                                    BOOLEAN truncateLob = TRUE ) ;

         INT32 truncateCollectionLoads( const CHAR *pName,
                                        dmsMBContext *context = NULL ) ;

         INT32 renameCollection ( const CHAR *oldName, const CHAR *newName,
                                  _pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                  BOOLEAN sysCollection = FALSE ) ;

         INT32 findCollection ( const CHAR *pName, UINT16 &collectionID ) ;

         INT32 insertRecord ( dmsMBContext *context,
                              const BSONObj &record,
                              _pmdEDUCB *cb,
                              SDB_DPSCB *dpscb,
                              BOOLEAN mustOID = TRUE,
                              BOOLEAN canUnLock = TRUE,
                              INT64 position = -1 ) ;

         INT32 deleteRecord ( dmsMBContext *context,
                              const dmsRecordID &recordID,
                              ossValuePtr deletedDataPtr,
                              _pmdEDUCB * cb,
                              SDB_DPSCB *dpscb ) ;

         INT32 updateRecord ( dmsMBContext *context,
                              const dmsRecordID &recordID,
                              ossValuePtr updatedDataPtr,
                              _pmdEDUCB *cb,
                              SDB_DPSCB *dpscb,
                              _mthModifier &modifier,
                              BSONObj* newRecord = NULL ) ;

         virtual INT32 popRecord( dmsMBContext *context,
                                  INT64 targetID,
                                  _pmdEDUCB *cb,
                                  SDB_DPSCB *dpscb,
                                  INT8 direction = 1 ) ;

         INT32 fetch ( dmsMBContext *context,
                       const dmsRecordID &recordID,
                       BSONObj &dataRecord,
                       _pmdEDUCB *cb,
                       BOOLEAN dataOwned = FALSE ) ;

         /* Create the compressor, and set the dictionry for it. */
         INT32 dictPersist( UINT16 mbID, UINT32 clLID, UINT32 startLID,
                            const CHAR *dict, UINT32 dictLen ) ;
         OSS_INLINE _dmsCompressorEntry *getCompressorEntry( UINT16 mbID ) ;

         /*
            Caller must hold the mbContext
         */
         virtual INT32 extractData( dmsMBContext *mbContext,
                                    dmsRecordRW &recordRW,
                                    _pmdEDUCB *cb,
                                    dmsRecordData &recordData ) = 0 ;

         virtual void postLoadExt( dmsMBContext *context,
                                   dmsExtent *extAddr,
                                   SINT32 extentID )
         {
         }

         virtual INT32 postDataRestored( dmsMBContext * context )
         {
            return SDB_OK ;
         }

         virtual INT32 dumpExtOptions( dmsMBContext *context,
                                       BSONObj &extOptions ) = 0 ;

      protected:
         virtual INT32 _prepareAddCollection( const BSONObj *extOption,
                                              dmsExtentID &extOptExtent,
                                              UINT16 &extentPageNum ) = 0 ;

         virtual INT32 _onAddCollection( const BSONObj *extOption,
                                         dmsExtentID extOptExtent,
                                         UINT32 extentSize,
                                         UINT16 collectionID ) = 0 ;

         virtual INT32 _onCollectionTruncated( dmsMBContext *context )
         {
            return SDB_OK ;
         }

         virtual void _onAllocExtent( dmsMBContext *context,
                                      dmsExtent *extAddr,
                                      SINT32 extentID ) = 0 ;

         virtual INT32 _prepareInsertData( const BSONObj &record,
                                           BOOLEAN mustOID,
                                           pmdEDUCB *cb,
                                           dmsRecordData &recordData,
                                           BOOLEAN &memReallocate,
                                           INT64 position ) = 0 ;

         virtual INT32 _allocRecordSpace( dmsMBContext *context,
                                          UINT32 size,
                                          dmsRecordID &foundRID,
                                          _pmdEDUCB *cb ) = 0 ;

         virtual INT32 _allocRecordSpaceByPos( dmsMBContext *context,
                                               UINT32 size,
                                               INT64 position,
                                               dmsRecordID &foundRID,
                                               _pmdEDUCB *cb ) = 0 ;

         virtual INT32 _extentInsertRecord( dmsMBContext *context,
                                            dmsExtRW &extRW,
                                            dmsRecordRW &recordRW,
                                            const dmsRecordData &recordData,
                                            UINT32 recordSize,
                                            _pmdEDUCB *cb,
                                            BOOLEAN isInsert = TRUE ) = 0 ;

         virtual INT32 _operationPermChk( DMS_ACCESS_TYPE accessType ) = 0 ;

         virtual INT32 _extentUpdatedRecord( dmsMBContext *context,
                                             dmsExtRW &extRW,
                                             dmsRecordRW &recordRW,
                                             const dmsRecordData &recordData,
                                             const BSONObj &newObj,
                                             _pmdEDUCB *cb ) = 0 ;

         virtual INT32 _extentRemoveRecord( dmsMBContext *context,
                                            dmsExtRW &extRW,
                                            dmsRecordRW &recordRW,
                                            _pmdEDUCB *cb,
                                            BOOLEAN decCount = TRUE ) = 0 ;

         virtual void _finalRecordSize( UINT32 &size,
                                        const dmsRecordData &recordData ) = 0 ;

         virtual INT32 _onInsertFail( dmsMBContext *context, BOOLEAN hasInsert,
                                      dmsRecordID rid, SDB_DPSCB *dpscb,
                                      ossValuePtr dataPtr, _pmdEDUCB *cb ) = 0 ;

         virtual INT32  _onOpened() ;
         virtual void   _onClosed() ;
         virtual INT32  _onMapMeta( UINT64 curOffSet ) ;
         virtual void   _onRestore() ;
         virtual INT32  _onFlushDirty( BOOLEAN force, BOOLEAN sync ) ;

      private:
         virtual UINT64 _dataOffset() ;
         virtual UINT32 _curVersion() const ;
         virtual INT32  _checkVersion( dmsStorageUnitHeader *pHeader ) ;
         virtual INT32  _onCreate( OSSFILE *file, UINT64 curOffSet ) ;

         virtual INT32  _onMarkHeaderValid( UINT64 &lastLSN,
                                            BOOLEAN sync,
                                            UINT64 lastTime ) ;

         virtual INT32  _onMarkHeaderInvalid( INT32 collectionID ) ;

         virtual UINT64 _getOldestWriteTick() const ;



      protected:
         OSS_INLINE const CHAR* _clFullName ( const CHAR *clName,
                                              CHAR *clFullName,
                                              UINT32 fullNameLen ) ;
         OSS_INLINE void _overflowSize( UINT32 &size ) ;

         void _attach ( _dmsStorageIndex *pIndexSu ) ;
         void _detach () ;

         void _attachLob( _dmsStorageLob *pLobSu ) ;
         void _detachLob() ;

         void _setCompressor( dmsMBContext *context ) ;
         void _rmCompressor( _dmsMBContext *context ) ;

         INT32 _allocateExtent ( dmsMBContext *context,
                                 UINT16 numPages,
                                 BOOLEAN deepInit = TRUE,
                                 BOOLEAN add2LoadList = FALSE,
                                 dmsExtentID *allocExtID = NULL ) ;

         INT32 _logDPS( SDB_DPSCB *dpsCB, dpsMergeInfo &info,
                        _pmdEDUCB *cb, dmsMBContext *context,
                        dmsExtentID extLID, BOOLEAN needUnLock,
                        DMS_FILE_TYPE type, UINT32 *clLID = NULL ) ;

         INT32 _freeExtent ( dmsExtentID extentID, INT32 collectionID ) ;
         INT32 _freeExtent ( dmsMBContext *context, dmsExtentID extentID ) ;

      private:
         void               _initializeMME () ;

         OSS_INLINE void    _collectionNameInsert ( const CHAR *pName,
                                                    UINT16 mbID ) ;
         OSS_INLINE UINT16  _collectionNameLookup ( const CHAR *pName ) ;
         OSS_INLINE void    _collectionNameRemove ( const CHAR *pName ) ;
         OSS_INLINE void    _collectionNameMapCleanup () ;

         INT32          _logDPS( SDB_DPSCB *dpsCB, dpsMergeInfo &info,
                                 _pmdEDUCB * cb, ossSLatch *pLatch,
                                 OSS_LATCH_MODE mode, BOOLEAN &locked,
                                 UINT32 clLID, dmsExtentID extLID ) ;

         INT32          _initCompressorEntry( UINT16 mbID ) ;

         INT32          _truncateCollection ( dmsMBContext *context,
                                              BOOLEAN needChangeCLID = TRUE ) ;

         INT32          _truncateCollectionLoads( dmsMBContext *context ) ;

         /*
            Caller must hold the mbContext
         */
         UINT32         _getRecordDataLen( const dmsRecord *pRecord ) ;

         OSS_INLINE UINT32  _getFactor () const ;

      protected:
         dmsMetadataManagementExtent         *_dmsMME ;     // 4MB

         ossSpinSLatch                       _mblock [ DMS_MME_SLOTS ] ;
         dmsMBStatInfo                       _mbStatInfo [ DMS_MME_SLOTS ] ;
         ossSpinSLatch                       _metadataLatch ;
         COLNAME_MAP                         _collectionNameMap ;
         UINT32                              _logicalCSID ;
         dmsStorageUnitID                    _CSID ;

         UINT32                              _mmeSegID ;

         vector<dmsMBContext*>               _vecContext ;
         ossSpinXLatch                       _latchContext ;

         _dmsStorageIndex                    *_pIdxSU ;
         _dmsStorageLob                      *_pLobSU ;

         _dmsCompressorEntry                 _compressorEntry[ DMS_MME_SLOTS ] ;

         _IDmsEventHolder                    *_pEventHolder ;
         _IDmsExtDataHandler                 *_pExtDataHandler ;

   };
   typedef _dmsStorageDataCommon dmsStorageDataCommon ;

   /*
      OSS_INLINE functions :
   */
   OSS_INLINE void _dmsStorageDataCommon::_collectionNameInsert( const CHAR * pName,
                                                                 UINT16 mbID )
   {
      _collectionNameMap[ ossStrdup( pName ) ] = mbID ;
   }
   OSS_INLINE UINT16 _dmsStorageDataCommon::_collectionNameLookup( const CHAR * pName )
   {
      COLNAME_MAP_CIT it = _collectionNameMap.find( pName ) ;
      if ( _collectionNameMap.end() == it  )
      {
         return DMS_INVALID_MBID ;
      }
      return (*it).second ;
   }
   OSS_INLINE void _dmsStorageDataCommon::_collectionNameRemove( const CHAR * pName )
   {
      COLNAME_MAP_IT it = _collectionNameMap.find( pName ) ;
      if ( _collectionNameMap.end() != it )
      {
         const CHAR *tp = (*it).first ;
         _collectionNameMap.erase( it ) ;
         SDB_OSS_FREE( const_cast<CHAR *>(tp) ) ;
      }
   }
   OSS_INLINE void _dmsStorageDataCommon::_collectionNameMapCleanup ()
   {
      COLNAME_MAP_CIT it = _collectionNameMap.begin() ;

      for ( ; it != _collectionNameMap.end() ; ++it )
      {
         SDB_OSS_FREE( const_cast<CHAR *>(it->first) ) ;
      }
      _collectionNameMap.clear() ;
   }
   OSS_INLINE UINT32 _dmsStorageDataCommon::getCollectionNum()
   {
      ossScopedLock lock( &_metadataLatch, SHARED ) ;
      return (UINT32)_collectionNameMap.size() ;
   }
   OSS_INLINE dmsRecordRW _dmsStorageDataCommon::record2RW( const dmsRecordID &record,
                                                            UINT16 collectionID )
   {
      dmsRecordRW rRW ;
      rRW._pData = this ;
      rRW._rid = record ;
      rRW._rw = extent2RW( record._extent, collectionID ) ;
      rRW._doneAddr() ;
      return rRW ;
   }
   OSS_INLINE const CHAR* _dmsStorageDataCommon::_clFullName( const CHAR *clName,
                                                              CHAR * clFullName,
                                                              UINT32 fullNameLen )
   {
      SDB_ASSERT( fullNameLen > DMS_COLLECTION_FULL_NAME_SZ,
                  "Collection full name len error" ) ;
      ossStrncat( clFullName, getSuName(), DMS_COLLECTION_SPACE_NAME_SZ ) ;
      ossStrncat( clFullName, ".", 1 ) ;
      ossStrncat( clFullName, clName, DMS_COLLECTION_NAME_SZ ) ;
      clFullName[ DMS_COLLECTION_FULL_NAME_SZ ] = 0 ;

      return clFullName ;
   }
   OSS_INLINE void _dmsStorageDataCommon::_overflowSize( UINT32 &size )
   {
      if ( _pStorageInfo && _pStorageInfo->_overflowRatio > 0 )
      {
         size += ( size * _pStorageInfo->_overflowRatio + 50 ) / 100 ;
      }
      else if ( !_pStorageInfo )
      {
         size = size * DMS_RECORD_OVERFLOW_RATIO ;
      }
   }
   OSS_INLINE void _dmsStorageDataCommon::updateCreateLobs( UINT32 createLobs )
   {
      if ( _dmsHeader && _dmsHeader->_createLobs != createLobs )
      {
         _dmsHeader->_createLobs = createLobs ;
         flushHeader( isSyncDeep() ) ;
      }
   }
   OSS_INLINE INT32 _dmsStorageDataCommon::getMBContext( dmsMBContext ** pContext,
                                                         UINT16 mbID,
                                                         UINT32 clLID,
                                                         UINT32 startLID,
                                                         INT32 lockType )
   {
      if ( mbID >= DMS_MME_SLOTS )
      {
         return SDB_INVALIDARG ;
      }

      if ( (UINT32)DMS_INVALID_CLID == clLID ||
           (UINT32)DMS_INVALID_CLID == startLID )
      {
         _metadataLatch.get_shared() ;
         if ( (UINT32)DMS_INVALID_CLID == clLID )
         {
            clLID = _dmsMME->_mbList[mbID]._logicalID ;
         }
         if ( (UINT32)DMS_INVALID_CLID == startLID )
         {
            startLID = _mbStatInfo[mbID]._startLID ;
         }
         _metadataLatch.release_shared() ;
      }

      _latchContext.get() ;
      if ( _vecContext.size () > 0 )
      {
         *pContext = _vecContext.back () ;
         _vecContext.pop_back () ;
      }
      else
      {
         *pContext = SDB_OSS_NEW dmsMBContext ;
      }
      _latchContext.release() ;

      if ( !(*pContext) )
      {
         return SDB_OOM ;
      }
      (*pContext)->_clLID = clLID ;
      (*pContext)->_startLID = startLID ;
      (*pContext)->_mbID = mbID ;
      (*pContext)->_mb = &_dmsMME->_mbList[mbID] ;
      (*pContext)->_mbStat = &_mbStatInfo[mbID] ;
      (*pContext)->_latch = &_mblock[mbID] ;
      if ( SHARED == lockType || EXCLUSIVE == lockType )
      {
         INT32 rc = (*pContext)->mbLock( lockType ) ;
         if ( rc )
         {
            releaseMBContext( *pContext ) ;
            return rc ;
         }
      }
      return SDB_OK ;
   }
   OSS_INLINE INT32 _dmsStorageDataCommon::getMBContext( dmsMBContext ** pContext,
                                                         const CHAR * pName,
                                                         INT32 lockType )
   {
      UINT16 mbID = DMS_INVALID_MBID ;
      UINT32 clLID = DMS_INVALID_CLID ;
      UINT32 startLID = DMS_INVALID_CLID ;

      _metadataLatch.get_shared() ;
      mbID = _collectionNameLookup( pName ) ;
      if ( DMS_INVALID_MBID != mbID )
      {
         clLID = _dmsMME->_mbList[mbID]._logicalID ;
         startLID = _mbStatInfo[mbID]._startLID ;
      }
      _metadataLatch.release_shared() ;

      if ( DMS_INVALID_MBID == mbID )
      {
         return SDB_DMS_NOTEXIST ;
      }
      return getMBContext( pContext, mbID, clLID, startLID, lockType ) ;
   }
   OSS_INLINE void _dmsStorageDataCommon::releaseMBContext( dmsMBContext *&pContext )
   {
      if ( !pContext )
      {
         return ;
      }
      pContext->mbUnlock() ;

      _latchContext.get() ;
      if ( _vecContext.size() < DMS_CONTEXT_MAX_SIZE )
      {
         pContext->_reset() ;
         _vecContext.push_back( pContext ) ;
      }
      else
      {
         SDB_OSS_DEL pContext ;
      }
      _latchContext.release() ;
      pContext = NULL ;
   }

   OSS_INLINE _dmsCompressorEntry *_dmsStorageDataCommon::getCompressorEntry( UINT16 mbID )
   {
      SDB_ASSERT( DMS_INVALID_MBID != mbID, "mb ID is invalid" ) ;
      return &_compressorEntry[ mbID ] ;
   }

   OSS_INLINE const dmsMBStatInfo* _dmsStorageDataCommon::getMBStatInfo( UINT16 mbID ) const
   {
      if ( mbID >= DMS_MME_SLOTS )
      {
         return NULL ;
      }
      return &_mbStatInfo[ mbID ] ;
   }

   OSS_INLINE const dmsMB* _dmsStorageDataCommon::getMBInfo( UINT16 mbID ) const
   {
      if ( !_dmsMME || mbID >= DMS_MME_SLOTS )
      {
         return NULL ;
      }
      return &(_dmsMME->_mbList[ mbID ]) ;
   }

   OSS_INLINE UINT32 _dmsStorageDataCommon::_getFactor() const
   {
      return 16 + 14 - pageSizeSquareRoot() ;
   }

   /*
      Tool Functions
   */
   BOOLEAN  dmsIsRecordIDValid( const BSONElement &oidEle,
                                BOOLEAN allowEOO,
                                const CHAR **pErrStr = NULL ) ;

}

#endif /* DMSSTORAGE_DATACOMMON_HPP_ */

