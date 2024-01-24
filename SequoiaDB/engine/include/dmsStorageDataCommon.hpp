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
#include "dpsTransVersionCtrl.hpp"
#include "dpsOp2Record.hpp"
#include "dmsCompress.hpp"
#include "dmsEventHandler.hpp"
#include "dmsExtDataHandler.hpp"
#include "dmsWriteGuard.hpp"
#include "ossMemPool.hpp"
#include "utilInsertResult.hpp"
#include "dmsOprHandler.hpp"
#include "monCB.hpp"
#include "sdbRemoteOperator.hpp"

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

#pragma pack(4)

   #define DMS_MB_SIZE                 (1024)

   /*
      _dmsMetadataBlock defined
   */
   struct _dmsMetadataBlock
   {
      // every records <= 32 bytes go to slot 0
      // every records >32 and <= 64 go to slot 1...
      // every records
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
      // for stat
      UINT64         _totalRecords ;
      UINT32         _totalDataPages ;
      UINT32         _totalIndexPages ;
      UINT64         _totalDataFreeSpace ;
      UINT64         _totalIndexFreeSpace ;
      UINT32         _totalLobPages ;
      // end

      // This extent is used to store dictionary of the collection. If the
      // dictionary has not been created, the value should be DMS_INVALID_EXTENT.
      dmsExtentID    _dictExtentID ;
      dmsExtentID    _newDictExtentID ;
      SINT32         _dictStatPageID ;
      UINT8          _dictVersion ;
      UINT8          _compressorType ;
      UINT8          _lastCompressRatio ;
      UINT8          _compressFlags ;
      // for stat
      UINT64         _totalLobs ;
      UINT64         _totalOrgDataLen ;
      UINT64         _totalDataLen ;
      // end stat

      // for persistence
      UINT64         _maxGlobTransID ;
      UINT32         _origLID ;
      utilCLInnerID  _origInnerUID ;
      UINT32         _commitFlag ;
      UINT64         _commitLSN ;
      UINT64         _commitTime ;
      UINT32         _idxCommitFlag ;
      UINT64         _idxCommitLSN ;
      UINT64         _idxCommitTime ;
      UINT32         _lobCommitFlag ;
      UINT64         _lobCommitLSN ;
      UINT64         _lobCommitTime ;
      // end persistence

      // Extend option extent id for collection.
      // If one storage type has its own special options, allocate one seperate
      // page to store them, instead of putting them in this common structure.
      dmsExtentID    _mbOptExtentID ;

      utilCLUniqueID _clUniqueID ;

      UINT64         _totalLobSize ;
      UINT64         _totalValidLobSize ;

      UINT64         _createTime ;
      UINT64         _updateTime ;

      // record ID generator
      UINT64         _ridGen ;

      // capped options
      INT64          _maxSize ;
      INT64          _maxRecNum ;
      BOOLEAN        _overwrite ;

      CHAR           _pad [ 216 ] ;

      void reset ( const CHAR *clName = NULL,
                   utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL,
                   UINT16 mbID = DMS_INVALID_MBID,
                   UINT32 clLID = DMS_INVALID_CLID,
                   UINT32 attr = 0,
                   UINT8 compressType = UTIL_COMPRESSOR_INVALID )
      {
         SDB_ASSERT( sizeof( _dmsMetadataBlock ) == DMS_MB_SIZE,
                     "metadata block header should be 1024" ) ;

         INT32 i = 0 ;
         ossMemset( _collectionName, 0, sizeof( _collectionName ) ) ;
         if ( clName )
         {
            ossStrncpy( _collectionName, clName, DMS_COLLECTION_NAME_SZ ) ;
         }
         _clUniqueID = clUniqueID ;
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
         _compressorType         = UTIL_COMPRESSOR_INVALID ;
         _dictVersion            = 0 ;
         _dictExtentID           = DMS_INVALID_EXTENT ;
         _newDictExtentID        = DMS_INVALID_EXTENT ;
         _dictStatPageID         = DMS_INVALID_EXTENT ;
         _lastCompressRatio      = 100 ;
         _compressFlags          = UTIL_COMPRESS_ALTERABLE_FLAG ;

         _totalOrgDataLen        = 0 ;
         _totalDataLen           = 0 ;
         _totalLobSize           = 0 ;
         _totalValidLobSize      = 0 ;

         _maxGlobTransID         = 0 ;
         _origLID                = clLID ;
         _origInnerUID                = utilGetCLInnerID( clUniqueID ) ;
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

         /// set compressor type
         if ( OSS_BIT_TEST( attr, DMS_MB_ATTR_COMPRESSED ) )
         {
            _compressorType      = compressType ;
         }

         _createTime             = 0 ;
         _updateTime             = 0 ;

         _ridGen                 = 0 ;

         _maxSize                = DMS_DFT_CAPPEDCL_SIZE ;
         _maxRecNum              = DMS_DFT_CAPPEDCL_RECNUM ;
         _overwrite              = FALSE ;

         // pad
         ossMemset( _pad, 0, sizeof( _pad ) ) ;
      }
   } ;
   typedef _dmsMetadataBlock  dmsMetadataBlock ;
   typedef dmsMetadataBlock   dmsMB ;

#pragma pack()

   // minimum space slot is 2^5 = 32 byte
   #define DMS_MIN_SPACE_SLOT_SQUQRE_ROOT ( 5 )

   // to avoid small deleted record which can not be reused by average
   // size of records in the same collection, we only split the record if
   // the remain size can at least save the record with average size,
   // or current record ( scale down to 0.8x )
   #define DMS_REMAIN_SIZE_RATIO          ( 0.8 )

   // get space slot to store the record with given size
   OSS_INLINE UINT8 dmsMBGetSpaceSlot( UINT32 recSize )
   {
      UINT8 freeSlot = 0 ;

      // divide by 32 (2^5) first since our first slot is for <32 bytes
      // while loop, divide by 2 every time, find the closest delete slot
      // for example, for a given size 3000, we should go _4k (which is
      // _deleteList[7], using 3000>>5=93
      // then in a loop, first round we have 46, type=1
      // then 23, type=2
      // then 11, type=3
      // then 5, type=4
      // then 2, type=5
      // then 1, type=6
      // finally 0, type=7
      recSize = ( recSize - 1 ) >> DMS_MIN_SPACE_SLOT_SQUQRE_ROOT ;
      while ( recSize != 0 )
      {
         ++ freeSlot ;
         recSize >>= 1 ;
      }

      return freeSlot ;
   }

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
      ossAtomic64 _totalRecords ;
      ossAtomic32 _writePtrCount ;
      UINT32      _totalDataPages ;
      UINT32      _totalIndexPages ;
      ossAtomic32 _totalLobPages ;
      UINT64      _totalDataFreeSpace ;
      UINT64      _totalIndexFreeSpace ;
      ossAtomic64 _totalLobs ;
      UINT8       _uniqueIdxNum ;
      UINT8       _textIdxNum ;
      UINT8       _globIdxNum ;
      UINT8       _lastCompressRatio ;
      ossAtomic64 _totalOrgDataLen ;
      ossAtomic64 _totalDataLen ;
      UINT32      _startLID ;
      UINT32      _flag ;
      ossAtomic64 _totalLobSize ;
      ossAtomic64 _totalValidLobSize ;

      ossAtomic32 _commitFlag ;
      ossAtomic64 _lastLSN ;
      // FIXME: need "atomic" for DPS_TRANS_ID later
      ossAtomic64 _maxGlobTransID ;
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
      // how many operators need to block index creating
      UINT32      _blockIndexCreatingCount ;

      // total record count for transaction RC count
      ossAtomic64 _rcTotalRecords ;

      // runtime CRUD statistics monitor
      monCRUDCB _crudCB ;

      // bitmap to indicate index fields
      ixmIdxHashBitmap _clIdxHashBitmap ;
      ixmIdxHashArray  _idxHashFields[ IXM_IDX_HASH_MAX_INDEX_NUM ] ;

      // the last search slot of delete list
      UINT8       _lastSearchSlot ;
      // the last search position of delete list
      dmsRecordID _lastSearchRID ;

      // cache of create time
      UINT64      _createTime ;
      // cache of update time
      UINT64      _updateTime ;

      // generator of record ID
      ossAtomic64 _ridGen ;

      // snapshot ID of metadata block
      ossAtomic64 _snapshotID ;

      void reset()
      {
         _totalRecords.init( 0 ) ;
         _writePtrCount.init( 0 ) ;
         _totalDataPages         = 0 ;
         _totalIndexPages        = 0 ;
         _totalDataFreeSpace     = 0 ;
         _totalIndexFreeSpace    = 0 ;
         _totalLobPages.init(0) ;
         _totalLobs.init(0) ;
         _uniqueIdxNum           = 0 ;
         _textIdxNum             = 0 ;
         _globIdxNum             = 0 ;
         _lastCompressRatio      = 100 ;
         _totalOrgDataLen.init( 0 ) ;
         _totalDataLen.init( 0 ) ;
         _startLID               = DMS_INVALID_CLID ;
         _flag                   = 0 ;
         _totalLobSize.init(0) ;
         _totalValidLobSize.init(0) ;
         _commitFlag.init( 0 ) ;
         _lastLSN.init( ~0 ) ;
         _lastWriteTick          = 0 ;
         _isCrash                = FALSE ;
         _idxCommitFlag.init( 0 ) ;
         _idxLastLSN.init( ~0 ) ;
         _maxGlobTransID.init( 0 ) ;
         _idxLastWriteTick       = 0 ;
         _idxIsCrash             = FALSE ;
         _lobCommitFlag.init( 0 ) ;
         _lobLastLSN.init( ~0 ) ;
         _lobLastWriteTick       = 0 ;
         _lobIsCrash             = FALSE ;
         _rcTotalRecords.init( 0 ) ;
         _crudCB.reset() ;
         _blockIndexCreatingCount = 0 ;
         _clIdxHashBitmap.resetBitmap() ;
         for ( UINT32 i = 0 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
         {
            _idxHashFields[ i ].reset() ;
         }
         _lastSearchSlot = dmsMB::_max ;
         _lastSearchRID.reset() ;
         _createTime             = 0 ;
         _updateTime             = 0 ;
         _ridGen.init( 0 ) ;
         _snapshotID.init( 0 ) ;
      }

      void updateLastLSN( UINT64 lsn, DMS_FILE_TYPE type )
      {
         if ( OSS_BIT_TEST( type, DMS_FILE_DATA ) )
         {
            _lastLSN.swap( lsn ) ;
         }
         if ( OSS_BIT_TEST( type, DMS_FILE_IDX ) )
         {
            _idxLastLSN.swap( lsn ) ;
         }
         if ( OSS_BIT_TEST( type, DMS_FILE_LOB ) )
         {
            _lobLastLSN.swap( lsn ) ;
         }
      }

      void updateLastLSNWithComp( UINT64 lsn,
                                  DMS_FILE_TYPE type,
                                  BOOLEAN isRollback )
      {
         if ( OSS_BIT_TEST( type, DMS_FILE_DATA ) )
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
         if ( OSS_BIT_TEST( type, DMS_FILE_IDX ) )
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
         if ( OSS_BIT_TEST( type, DMS_FILE_LOB ) )
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

      // compare and update GlobTransID is the one passed in is newer
      // FIXME: need implement automic compare and swap for DPS_TRANS_ID later
      void updateGlobTransIDWithComp( DPS_TRANS_ID transID )
      {
         _maxGlobTransID.swapGreaterThan( transID ) ;
      }

      // compare and update GTID is the one passed in is newer
      UINT64 getMaxGlobTransID( )
      {
         return _maxGlobTransID.peek() ;
      }

      void setIdxHash( INT32 indexID, const CHAR *idxFieldName )
      {
         SDB_ASSERT( indexID >= 0 && indexID < DMS_COLLECTION_MAX_INDEX,
                     "invalid index ID" ) ;
         UINT32 bitIndex = ixmIdxHashBitmap::calcIndex( idxFieldName ) ;
         _clIdxHashBitmap.setBit( bitIndex ) ;
         if ( indexID < IXM_IDX_HASH_MAX_INDEX_NUM )
         {
            _idxHashFields[ indexID ].setField( bitIndex ) ;
         }
      }

      // reset index hash fields from given index
      void resetIdxHashFrom( INT32 indexID )
      {
         SDB_ASSERT( indexID >= 0 && indexID < DMS_COLLECTION_MAX_INDEX,
                     "invalid index ID" ) ;
         _clIdxHashBitmap.resetBitmap() ;
         // reset bitmaps after given index ID
         for ( UINT32 i = indexID ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
         {
            _idxHashFields[ i ].reset() ;
         }
      }

      void resetIdxHashAt( INT32 indexID )
      {
         SDB_ASSERT( indexID >= 0 && indexID < DMS_COLLECTION_MAX_INDEX,
                     "invalid index ID" ) ;
         if ( indexID < IXM_IDX_HASH_MAX_INDEX_NUM )
         {
            _idxHashFields[ indexID ].reset() ;
         }
      }

      void mergeIdxHash( INT32 indexID )
      {
         SDB_ASSERT( indexID >= 0 && indexID < DMS_COLLECTION_MAX_INDEX,
                     "invalid index ID" ) ;
         if ( indexID < IXM_IDX_HASH_MAX_INDEX_NUM )
         {
            _idxHashFields[ indexID ].mergeToBitmap( _clIdxHashBitmap ) ;
         }
      }

      BOOLEAN testIdxHash( const ixmIdxHashBitmap &idxHash )
      {
         return _clIdxHashBitmap.hasIntersaction( idxHash ) ;
      }

      BOOLEAN testIdxHash( INT32 indexID, const ixmIdxHashBitmap &idxHash )
      {
         SDB_ASSERT( indexID >= 0 && indexID < DMS_COLLECTION_MAX_INDEX,
                     "invalid index ID" ) ;
         if ( indexID < IXM_IDX_HASH_MAX_INDEX_NUM )
         {
            return _idxHashFields[ indexID ].testBitmap( idxHash ) ;
         }
         return TRUE ;
      }

      BOOLEAN isIdxHashReady() const
      {
         return !( _clIdxHashBitmap.isEmpty() ) ;
      }

      BOOLEAN isIdxHashReady( INT32 indexID ) const
      {
         SDB_ASSERT( indexID >= 0 && indexID < DMS_COLLECTION_MAX_INDEX,
                     "invalid index ID" ) ;
         if ( indexID < IXM_IDX_HASH_MAX_INDEX_NUM )
         {
            return _idxHashFields[ indexID ].isValid() ;
         }
         // for indexes after first 8 ones, always not ready
         return FALSE ;
      }

      UINT32 getAvgDataSize()
      {
         UINT64 recNum = _totalRecords.fetch() ;
         if ( 0 != recNum )
         {
            // calculate from total data length and total records
            UINT64 avgSize = _totalDataLen.fetch() / recNum ;
            avgSize = OSS_MAX( DMS_MIN_RECORD_SZ, avgSize ) ;
            avgSize = OSS_MIN( DMS_RECORD_USER_MAX_SZ, avgSize ) ;
            return (UINT32)( avgSize ) ;
         }
         return 0 ;
      }

      void addTotalLobSize( INT64 size )
      {
         ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalLobSize ), size ) ;
      }

      void subTotalLobSize( INT64 size )
      {
         addTotalLobSize( 0 - size ) ;
      }

      void resetTotalLobSize()
      {
         ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalLobSize ), 0 ) ;
      }

      void addTotalValidLobSize( INT64 size )
      {
         ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalValidLobSize ), size ) ;
      }

      void resetTotalValidLobSize()
      {
         ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalValidLobSize ), 0 ) ;
      }

      _dmsMBStatInfo ()
      : _totalRecords( 0 ),
        _writePtrCount( 0 ),
        _totalLobPages(0),
        _totalLobs(0),
        _totalOrgDataLen( 0 ),  
        _totalDataLen( 0 ),
        _totalLobSize(0),
        _totalValidLobSize(0),
        _commitFlag( 0 ),
        _lastLSN( 0 ),
        _maxGlobTransID( 0 ),
        _idxCommitFlag( 0 ),
        _idxLastLSN( 0 ),
        _lobCommitFlag( 0 ),
        _lobLastLSN( 0 ),
        _rcTotalRecords( 0 ),
        _ridGen( 0 ),
        _snapshotID( 0 )
      {
         reset() ;
      }

      ~_dmsMBStatInfo ()
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

         void setSubContext( _IContext *subContext ) ;
         void swap( _dmsMBContext &other ) ;

         OSS_INLINE INT32   mbLock( INT32 lockType ) ;
         OSS_INLINE INT32   mbTryLock( INT32 lockType ) ;
         OSS_INLINE INT32   mbUnlock() ;
         OSS_INLINE BOOLEAN isMBLock( INT32 lockType ) const ;
         OSS_INLINE BOOLEAN isMBLock() const ;
         OSS_INLINE BOOLEAN canResume() const ;

         virtual     UINT16 mbID () const { return _mbID ; }
         OSS_INLINE  dmsMB* mb () { return _mb ; }
         OSS_INLINE  const dmsMB* mb() const { return _mb ; }
         OSS_INLINE  dmsMBStatInfo* mbStat() { return _mbStat ; }
         OSS_INLINE  const dmsMBStatInfo* mbStat() const { return _mbStat ; }
         OSS_INLINE  UINT32 clLID () const { return _clLID ; }
         OSS_INLINE  UINT32 startLID() const { return _startLID ; }
         OSS_INLINE  INT32  mbLockType() const { return _mbLockType ; }

         OSS_INLINE const CHAR *clName() const
         {
            return _mb ? _mb->_collectionName : NULL ;
         }

         OSS_INLINE std::shared_ptr<ICollection> &getCollPtr() const
         {
            return _collPtr ;
         }

         OSS_INLINE utilCLUniqueID getCLUniqueID() const
         {
            return _mb ? _mb->_clUniqueID : UTIL_UNIQUEID_NULL ;
         }

      private:
         OSS_INLINE INT32   _mbLock( INT32 lockType, BOOLEAN isTry ) ;
      private:
         dmsMB             *_mb ;
         dmsMBStatInfo     *_mbStat ;
         monSpinSLatch     *_latch ;
         UINT32            _clLID ;
         UINT32            _startLID ;
         UINT16            _mbID ;
         INT32             _mbLockType ;
         INT32             _resumeType ;
         _IContext         *_pSubContext ;
         mutable std::shared_ptr<ICollection> _collPtr ;
   };
   typedef _dmsMBContext   dmsMBContext ;

   class _dmsMBContextSubScope : public SDBObject
   {
   public:
      _dmsMBContextSubScope( _dmsMBContext* mbContext, _IContext *subContext ) ;
      ~_dmsMBContextSubScope() ;

   private:
      _dmsMBContext *_mbContext ;
   } ;

   /*
      _dmsMBContext OSS_INLINE functions
   */
   OSS_INLINE INT32 _dmsMBContext::_mbLock( INT32 lockType,
                                            BOOLEAN isTry )
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
      // already lock(type not same), need to unlock
      if ( -1 != _mbLockType && SDB_OK != ( rc = pause() ) )
      {
         return rc ;
      }

      // check before lock
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
      else if ( _startLID != _mbStat->_startLID )
      {
         // start logical IDs are different
         // NOTE: recycle or return cases will keep the logical ID
         if ( (UINT32)DMS_INVALID_CLID == _mbStat->_startLID )
         {
            // collection is recycled by drop or truncate
            if ( DMS_MB_STATINFO_IS_TRUNCATED( _mbStat->_flag) )
            {
               return SDB_DMS_TRUNCATED ;
            }
            else
            {
               return SDB_DMS_NOTEXIST ;
            }
         }
         else if ( (UINT32)DMS_INVALID_CLID == _startLID )
         {
            // recycle collection is returned
            return SDB_RECYCLE_ITEM_NOTEXIST ;
         }
         return SDB_DMS_NOTEXIST ;
      }

      if ( isTry )
      {
         BOOLEAN hasLock = FALSE ;
         hasLock = ( SHARED == lockType ) ?
                   _latch->try_get_shared() : _latch->try_get() ;
         if ( !hasLock )
         {
            return SDB_TIMEOUT ;
         }
      }
      else
      {
         ossLatch( _latch, (OSS_LATCH_MODE)lockType ) ;
      }

      // check after lock
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
      else if ( _startLID != _mbStat->_startLID )
      {
         // start logical IDs are different
         // NOTE: recycle or return cases will keep the logical ID
         if ( (UINT32)DMS_INVALID_CLID == _mbStat->_startLID )
         {
            // collection is recycled by drop or truncate
            if ( DMS_MB_STATINFO_IS_TRUNCATED( _mbStat->_flag) )
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
         else if ( (UINT32)DMS_INVALID_CLID == _startLID )
         {
            // recycle collection is returned
            ossUnlatch( _latch, (OSS_LATCH_MODE)lockType ) ;
            return SDB_RECYCLE_ITEM_NOTEXIST ;
         }
         ossUnlatch( _latch, (OSS_LATCH_MODE)lockType ) ;
         return SDB_DMS_NOTEXIST ;
      }

      _mbLockType = lockType ;
      _resumeType = -1 ;
      return SDB_OK ;
   }
   OSS_INLINE INT32 _dmsMBContext::mbLock( INT32 lockType )
   {
      return _mbLock( lockType, FALSE ) ;
   }
   OSS_INLINE INT32 _dmsMBContext::mbTryLock( INT32 lockType )
   {
      return _mbLock( lockType, TRUE ) ;
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
         _dmsRecordRW( const _dmsRecordRW &recordRW ) ;
         virtual ~_dmsRecordRW() ;

         BOOLEAN           isEmpty() const ;
         _dmsRecordRW      derivePre() const ;
         _dmsRecordRW      deriveNext() const ;
         _dmsRecordRW      deriveOverflow() const ;
         _dmsRecordRW      derive( const dmsRecordID &rid ) const ;

         void              setNothrow( BOOLEAN nothrow ) ;
         BOOLEAN           isNothrow() const ;

         BOOLEAN           isDirectMem() const { return _isDirectMem ; }

         dmsRecordID       getRecordID() const { return _rid ; }

         /*
            When len == 0, Use the record's size
         */
         const dmsRecord*  readPtr( UINT32 len = sizeof( dmsRecord ) ) const ;
         dmsRecord*        writePtr( UINT32 len = sizeof( dmsRecord ) ) ;


         template< typename T >
         const T*          readPtr( UINT32 len = sizeof(T) ) const
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

      protected:
         BOOLEAN           _isDirectMem ;
         const dmsRecord   *_ptr ;

      private:
         dmsRecordID       _rid ;
         dmsExtRW          _rw ;
         _dmsStorageDataCommon   *_pData ;
   } ;
   typedef class _dmsRecordRW dmsRecordRW ;

   #define DMS_MME_OFFSET                 ( DMS_SME_OFFSET + DMS_SME_SZ )
   #define DMS_DATASU_EYECATCHER          "SDBDATA"

   // History of data version change:
   // Version  Update in which version    Reason
   //    1             --                 The Initial version.
   //    2             2.0                Support for lzw compression. New
   //                                     dictionary extent information added
   //                                     in MB.
   //    3             2.9                Support for capped collection. A new
   //                                     page for extend option is used, id
   //                                     stored in MB.
   //    16            5.0.3/5.0.4        Support for MVCC.
   // WARNING: next version should start from 17
   #define DMS_COMPRESSION_ENABLE_VER     2
   #define DMS_CAPPED_ENABLE_VER          3
   #define DMS_MVCC_ENABLE_VER            16
   #define DMS_DATASU_CUR_VERSION         DMS_CAPPED_ENABLE_VER
   #define DMS_DATASU_MAX_VERSION         DMS_MVCC_ENABLE_VER

   #define DMS_DATACAPSU_EYECATCHER       "SDBDCAP"
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
      friend class _dmsRBSSUMgr ;


      struct cmp_str
      {
         bool operator() (const char *a, const char *b)
         {
            return ossStrcmp( a, b ) < 0 ;
         }
      } ;

      typedef ossPoolMap<const CHAR*, UINT16, cmp_str> COLNAME_MAP ;
      typedef ossPoolMap<utilCLUniqueID, UINT16>       COLID_MAP ;
#if defined (_WINDOWS)
      typedef COLNAME_MAP::iterator                         COLNAME_MAP_IT ;
      typedef COLNAME_MAP::const_iterator                   COLNAME_MAP_CIT ;
#else
      typedef ossPoolMap<const CHAR*, UINT16>::iterator       COLNAME_MAP_IT ;
      typedef ossPoolMap<const CHAR*, UINT16>::const_iterator COLNAME_MAP_CIT ;
#endif
      typedef COLID_MAP::iterator    COLID_MAP_IT ;
      typedef COLID_MAP::const_iterator COLID_MAP_CIT ;

      public:
         _dmsStorageDataCommon ( IStorageService *service,
                                 dmsSUDescriptor *suDescriptor,
                                 const CHAR *pSuFileName,
                                 _IDmsEventHolder *pEventHolder ) ;
         virtual ~_dmsStorageDataCommon () ;

         virtual void  syncMemToMmap() ;

         UINT32 logicalID () const { return _logicalCSID ; }
         dmsStorageUnitID CSID () const { return _CSID ; }

         OSS_INLINE INT32  getMBContext( dmsMBContext **pContext, UINT16 mbID,
                                         UINT32 clLID, UINT32 startLID,
                                         INT32 lockType = -1 );
         OSS_INLINE INT32  getMBContext( dmsMBContext **pContext,
                                         const CHAR* pName,
                                         INT32 lockType = -1 ) ;
         OSS_INLINE INT32  getMBContextByID( dmsMBContext **pContext,
                                             utilCLUniqueID clUniqueID,
                                             INT32 lockType = -1 ) ;

         OSS_INLINE INT32  checkMBContext( const CHAR *pName, UINT16 mbID ) ;
         OSS_INLINE void   releaseMBContext( dmsMBContext *&pContext ) ;

         OSS_INLINE dmsMBStatInfo* getMBStatInfo( UINT16 mbID ) ;
         OSS_INLINE const dmsMB* getMBInfo( UINT16 mbID ) const ;

         OSS_INLINE UINT32 getCollectionNum() ;

         OSS_INLINE dmsRecordRW record2RW( const dmsRecordID &record,
                                           UINT16 collectionID ) const ;

         OSS_INLINE INT32 getMBInfo( const CHAR *pName,
                                     UINT16 &mbID,
                                     UINT32 &clLID,
                                     utilCLUniqueID &clUniqueID ) ;

         BOOLEAN isCapped () { return _isCapped; }

         // update extent logical id and expanded meta
         // must hold mb exclusive lock
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

         /// flush mme
         INT32          flushMME( BOOLEAN sync = FALSE ) ;

         BOOLEAN        isTransSupport( dmsMBContext *context ) const ;
         BOOLEAN        isTransLockRequired( dmsMBContext *context ) const ;

      public:

         // create a new collection for given name, returns collectionID
         INT32 addCollection ( const CHAR *pName,
                               UINT16 *collectionID,
                               utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL,
                               UINT32 attributes = 0,
                               _pmdEDUCB * cb = NULL,
                               SDB_DPSCB *dpscb = NULL,
                               UINT16 initPages = 0,
                               BOOLEAN sysCollection = FALSE,
                               UINT8 compressionType = UTIL_COMPRESSOR_INVALID,
                               UINT32 *logicID = NULL,
                               const BSONObj *extOptions = NULL,
                               const BSONObj *pIdIdxDef = NULL,
                               BOOLEAN addIdxIDIfNotExist = FALSE ) ;

         INT32 dropCollection ( const CHAR *pName,
                                _pmdEDUCB *cb,
                                SDB_DPSCB *dpscb,
                                BOOLEAN sysCollection = TRUE,
                                dmsMBContext *context = NULL,
                                dmsDropCLOptions *options = NULL ) ;

         INT32 truncateCollection ( const CHAR *pName,
                                    _pmdEDUCB *cb,
                                    SDB_DPSCB *dpscb,
                                    BOOLEAN sysCollection = TRUE,
                                    dmsMBContext *context = NULL,
                                    BOOLEAN needChangeCLID = TRUE,
                                    BOOLEAN truncateLob = TRUE,
                                    dmsTruncCLOptions *options = NULL ) ;

         INT32 prepareCollectionLoads( dmsMBContext *context,
                                       const bson::BSONObj &record,
                                       BOOLEAN isLast,
                                       BOOLEAN isAsynchr,
                                       pmdEDUCB *cb ) ;
         INT32 truncateCollectionLoads( dmsMBContext *context,
                                        pmdEDUCB *cb ) ;
         INT32 buildCollectionLoads( dmsMBContext *context,
                                     BOOLEAN isAsynchr,
                                     pmdEDUCB *cb ) ;

         INT32 changeCLUniqueID( const MAP_CLNAME_ID& modifyCl,
                                 BOOLEAN changeOtherCL,
                                 utilCSUniqueID csUniqueID,
                                 BOOLEAN isLoadCS,
                                 ossPoolVector<ossPoolString>& clVec ) ;

         INT32 renameCollection ( const CHAR *oldName, const CHAR *newName,
                                  _pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                  BOOLEAN sysCollection = FALSE,
                                  utilCLUniqueID newCLUniqueID = UTIL_UNIQUEID_NULL,
                                  UINT32 *newStartLID = NULL ) ;

         INT32 copyCollection( dmsMBContext *mbContext,
                               const CHAR *newName,
                               utilCLUniqueID newCLUniqueID,
                               pmdEDUCB *cb ) ;
         INT32 recycleCollection( dmsMBContext *mbContext, pmdEDUCB *cb ) ;

         INT32 returnCollection( const CHAR *originName,
                                 const CHAR *recycleName,
                                 dmsReturnOptions &options,
                                 pmdEDUCB *cb,
                                 SDB_DPSCB *dpsCB,
                                 dmsMBContext **returnedMBContext ) ;

         INT32 findCollection ( const CHAR *pName,
                                UINT16 &collectionID,
                                utilCLUniqueID *pClUniqueID = NULL ) ;

         INT32 insertRecord ( dmsMBContext *context,
                              const BSONObj &record,
                              _pmdEDUCB *cb,
                              SDB_DPSCB *dpscb,
                              BOOLEAN mustOID = TRUE,
                              BOOLEAN canUnLock = TRUE,
                              INT64 position = -1,
                              utilInsertResult *insertResult = NULL ) ;

         // if deletedDataPtr = 0, will get from recordID
         // must hold mb exclusive lock
         INT32 deleteRecord ( dmsMBContext *context,
                              const dmsRecordID &recordID,
                              ossValuePtr deletedDataPtr,
                              _pmdEDUCB * cb,
                              SDB_DPSCB *dpscb,
                              IDmsOprHandler *pHandler = NULL,
                              const dmsTransRecordInfo *pInfo = NULL,
                              BOOLEAN isUndo = FALSE ) ;

         // if updatedDataPtr = 0, will get from recordID
         // must hold mb exclusive lock
         INT32 updateRecord ( dmsMBContext *context,
                              const dmsRecordID &recordID,
                              ossValuePtr updatedDataPtr,
                              _pmdEDUCB *cb,
                              SDB_DPSCB *dpscb,
                              _mthModifier &modifier,
                              BSONObj* newRecord = NULL,
                              IDmsOprHandler *pHandler = NULL,
                              utilUpdateResult *pResult = NULL ) ;

         virtual INT32 popRecord( dmsMBContext *context,
                                  INT64 targetID,
                                  _pmdEDUCB *cb,
                                  SDB_DPSCB *dpscb,
                                  INT8 direction = 1,
                                  BOOLEAN byNumber = FALSE ) ;

         // the dataRecord is not owned
         // Caller must hold mb exclusive/shared lock
         INT32 fetch ( dmsMBContext *context,
                       const dmsRecordID &recordID,
                       dmsRecordData &recordData,
                       _pmdEDUCB *cb ) ;

         virtual void incWritePtrCount( INT32 collectionID ) ;

         virtual void decWritePtrCount( INT32 collectionID ) ;

         /*
            Caller must hold the mbContext
         */
         virtual INT32 extractData( const dmsMBContext *mbContext,
                                    const dmsRecordID &recordID,
                                    _pmdEDUCB *cb,
                                    dmsRecordData &recordData,
                                    BOOLEAN needIncDataRead = TRUE,
                                    BOOLEAN needGetOwned = TRUE ) ;

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

         virtual INT32 setExtOptions ( dmsMBContext * context,
                                       const BSONObj & extOptions ) = 0 ;

         virtual OSS_LATCH_MODE getWriteLockType() const = 0 ;

         void increaseMBStat ( utilCLUniqueID clUniqueID,
                               dmsMBStatInfo * mbStat,
                               UINT64 delta,
                               _pmdEDUCB * cb ) ;
         void decreaseMBStat ( utilCLUniqueID clUniqueID,
                               dmsMBStatInfo * mbStat,
                               UINT64 delta,
                               _pmdEDUCB * cb ) ;

      protected:
         virtual INT32 _prepareAddCollection( const BSONObj *extOption,
                                              dmsCreateCLOptions &options ) = 0 ;

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

         virtual INT32 _checkInsertData( const BSONObj &record,
                                         BOOLEAN mustOID,
                                         pmdEDUCB *cb,
                                         dmsRecordData &recordData,
                                         BOOLEAN &memReallocate,
                                         INT64 position ) = 0 ;

         virtual INT32 _prepareInsert( const dmsRecordID &recordID,
                                       const dmsRecordData &recordData ) = 0 ;

         virtual INT32 _getRecordPosition( dmsMBContext *context,
                                           const dmsRecordID &rid,
                                           const dmsRecordData &recordData,
                                           pmdEDUCB *cb,
                                           INT64 &position ) = 0 ;

         virtual INT32 _checkReusePosition( dmsMBContext *context,
                                            const DPS_TRANS_ID &transID,
                                            pmdEDUCB *cb,
                                            INT64 &position,
                                            dmsRecordID &foundRID ) = 0 ;

         virtual INT32 _allocRecordSpace( dmsMBContext *context,
                                          UINT32 size,
                                          dmsRecordID &foundRID,
                                          _pmdEDUCB *cb ) = 0 ;

         virtual INT32 _checkRecordSpace( dmsMBContext *context,
                                          UINT32 size,
                                          dmsRecordID &foundRID,
                                          _pmdEDUCB *cb ) = 0 ;

         virtual INT32 _operationPermChk( DMS_ACCESS_TYPE accessType ) = 0 ;

         // Calculate the final size needed by the record. Records of different
         // type may have different strategy, such as reservation for update,
         // space for meta data, etc.
         virtual void _finalRecordSize( UINT32 &size,
                                        const dmsRecordData &recordData ) = 0 ;

         virtual INT32  _onOpened() ;
         virtual void   _onClosed() ;
         virtual INT32  _onMapMeta( UINT64 curOffSet ) ;
         virtual void   _onRestore() ;
         virtual INT32  _onFlushDirty( BOOLEAN force, BOOLEAN sync ) ;

         virtual void   _onHeaderUpdated( UINT64 updateTime = 0 )
         {
            _dmsStorageBase::_onHeaderUpdated( updateTime ) ;
            if ( NULL != _dmsHeader && NULL != _suDescriptor )
            {
               _suDescriptor->getStorageInfo()._updateTime = _dmsHeader->_updateTime ;
            }
         }

         INT32 _copyIndexesWithoutTypes( dmsMBContext *oldContext,
                                         dmsMBContext *newContext,
                                         _pmdEDUCB *cb,
                                         UINT16 types ) ;
         INT32 _dropIndexesWithTypes( dmsMBContext *context,
                                      _pmdEDUCB *cb,
                                      UINT16 types,
                                      ossPoolVector< bson::BSONObj > *droppedIndexList = NULL ) ;

      private:
         virtual UINT64 _dataOffset() ;
         virtual UINT32 _curVersion() const ;
         virtual UINT32 _maxSupportedVersion() const ;
         virtual INT32  _checkVersion( dmsStorageUnitHeader *pHeader ) ;
         virtual INT32  _onCreate( OSSFILE *file, UINT64 curOffSet ) ;

         virtual INT32  _onMarkHeaderValid( UINT64 &lastLSN,
                                            BOOLEAN sync,
                                            UINT64 lastTime,
                                            BOOLEAN &setHeadCommFlgValid ) ;

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

         // This function allocates a new extent. When the extent is allocated,
         // different storage types( sub classes of this base class ) may have
         // different further in-extent initialize operations. The parameter
         // 'deepInit' specifies if those operations should happen.
         INT32 _allocateExtent ( dmsMBContext *context,
                                 UINT16 numPages,
                                 BOOLEAN deepInit = TRUE,
                                 BOOLEAN add2LoadList = FALSE,
                                 dmsExtentID *allocExtID = NULL ) ;

         INT32 _logDPS( SDB_DPSCB *dpsCB, dpsMergeInfo &info,
                        _pmdEDUCB *cb, dmsMBContext *context,
                        dmsExtentID extID, dmsOffset extOffset, BOOLEAN needUnLock,
                        DMS_FILE_TYPE type, UINT32 *clLID = NULL ) ;

         INT32 _freeExtent ( dmsExtentID extentID, INT32 collectionID ) ;
         INT32 _freeExtent ( dmsMBContext *context, dmsExtentID extentID ) ;

         void _onMBUpdated( UINT16 mbID ) ;

      private:
         void               _initializeMME () ;

         OSS_INLINE INT32 _collectionInsert( const CHAR *pName,
                                             UINT16 mbID,
                                             utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ) ;

         OSS_INLINE UINT16 _collectionNameLookup( const CHAR *pName ) ;
         OSS_INLINE UINT16 _collectionIdLookup( utilCLUniqueID clUniqueID ) ;

         OSS_INLINE void _collectionRemove( const CHAR *pName,
                                            utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ) ;
         OSS_INLINE void _collectionMapCleanup () ;

         INT32          _logDPS( SDB_DPSCB *dpsCB, dpsMergeInfo &info,
                                 _pmdEDUCB * cb, ossSLatch *pLatch,
                                 OSS_LATCH_MODE mode, BOOLEAN &locked,
                                 UINT32 clLID, dmsExtentID extID,
                                 dmsOffset extOffset ) ;

         INT32          _truncateCollection ( dmsMBContext *context,
                                              BOOLEAN needChangeCLID = TRUE ) ;

         OSS_INLINE UINT32  _getFactor () const ;

         INT32          _insertIndexes( dmsMBContext *context,
                                        dmsExtentID extLID,
                                        BSONObj &inputObj,
                                        const dmsRecordID &rid,
                                        pmdEDUCB * cb,
                                        IDmsOprHandler *pOprHandle,
                                        dmsWriteGuard &writeGuard,
                                        utilWriteResult *insertResult,
                                        dpsUnqIdxHashArray *pUnqIdxHashArray ) ;

      //private:
      protected:
         dmsMetadataManagementExtent         *_dmsMME ;     // 4MB

         // latch for each MB. For normal record SIUD, shared latches are
         // requested exclusive latch on mblock is only when changing
         // metadata (say add an extent into the MB, or create/drop the MB)
         monSpinSLatch                       _mblock [ DMS_MME_SLOTS ] ;
         ossSpinXLatch                       _mbStatLatch ;
         dmsMBStatInfo                       _mbStatInfo [ DMS_MME_SLOTS ] ;
         monSpinSLatch                       _metadataLatch ;
         COLNAME_MAP                         _collectionNameMap ;
         COLID_MAP                           _collectionIDMap ;
         UINT32                              _logicalCSID ;
         dmsStorageUnitID                    _CSID ;

         UINT32                              _mmeSegID ;
         BOOLEAN                             _isCapped ;

         vector<dmsMBContext*>               _vecContext ;
         monSpinXLatch                       _latchContext ;

         _dmsStorageIndex                    *_pIdxSU ;
         _dmsStorageLob                      *_pLobSU ;

         _IDmsEventHolder                    *_pEventHolder ;
         _IDmsExtDataHandler                 *_pExtDataHandler ;

   };
   typedef _dmsStorageDataCommon dmsStorageDataCommon ;

   /*
      OSS_INLINE functions :
   */
   OSS_INLINE INT32 _dmsStorageDataCommon::_collectionInsert( const CHAR * pName,
                                                              UINT16 mbID,
                                                              utilCLUniqueID clUniqueID )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isInserted = FALSE ;
      CHAR *clNamePtr = ossStrdup( pName ) ;
      if ( NULL == clNamePtr )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate dup string failed." ) ;
         goto error ;
      }

      try
      {
         _collectionNameMap[ clNamePtr ] = mbID ;
         isInserted = TRUE ;
         if ( UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
         {
            _collectionIDMap[ clUniqueID ] = mbID ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Insert mbid into maps failed, exception:%s, rc:%d.",
                 e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( isInserted )
      {
         _collectionNameMap.erase( pName ) ;
      }
      SAFE_OSS_FREE( clNamePtr ) ;
      goto done ;
   }
   OSS_INLINE UINT16 _dmsStorageDataCommon::_collectionNameLookup( const CHAR * pName )
   {
      UINT16 mbID = DMS_INVALID_MBID ;
      if ( pName )
      {
         COLNAME_MAP_CIT it = _collectionNameMap.find( pName ) ;
         if ( it != _collectionNameMap.end() )
         {
            mbID = it->second ;
         }
      }
      return mbID ;
   }
   OSS_INLINE UINT16 _dmsStorageDataCommon::_collectionIdLookup( utilCLUniqueID clUniqueID )
   {
      UINT16 mbID = DMS_INVALID_MBID ;
      if ( UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
      {
         COLID_MAP_CIT it = _collectionIDMap.find( clUniqueID ) ;
         if ( it != _collectionIDMap.end() )
         {
            mbID = it->second ;
         }
      }
      return mbID ;
   }
   OSS_INLINE void _dmsStorageDataCommon::_collectionRemove( const CHAR * pName,
                                                             utilCLUniqueID clUniqueID )
   {
      COLNAME_MAP_IT it = _collectionNameMap.find( pName ) ;
      if ( _collectionNameMap.end() != it )
      {
         const CHAR *tp = (*it).first ;
         _collectionNameMap.erase( it ) ;
         SDB_OSS_FREE( const_cast<CHAR *>(tp) ) ;
      }

      if ( UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
      {
         _collectionIDMap.erase( clUniqueID ) ;
      }
   }
   OSS_INLINE void _dmsStorageDataCommon::_collectionMapCleanup ()
   {
      COLNAME_MAP_CIT it = _collectionNameMap.begin() ;

      for ( ; it != _collectionNameMap.end() ; ++it )
      {
         SDB_OSS_FREE( const_cast<CHAR *>(it->first) ) ;
      }
      _collectionNameMap.clear() ;
      _collectionIDMap.clear() ;
   }
   OSS_INLINE UINT32 _dmsStorageDataCommon::getCollectionNum()
   {
      ossScopedLock lock( &_metadataLatch, SHARED ) ;
      return (UINT32)_collectionNameMap.size() ;
   }
   OSS_INLINE dmsRecordRW _dmsStorageDataCommon::record2RW( const dmsRecordID &record,
                                                            UINT16 collectionID ) const
   {
      dmsRecordRW rRW ;
      rRW._pData = const_cast<_dmsStorageDataCommon*>( this ) ;
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
      if ( _suDescriptor && _suDescriptor->getStorageInfo()._overflowRatio > 0 )
      {
         size += ( size * _suDescriptor->getStorageInfo()._overflowRatio + 50 ) / 100 ;
      }
      else if ( !_suDescriptor )
      {
         size = size * DMS_RECORD_OVERFLOW_RATIO ;
      }
   }
   OSS_INLINE void _dmsStorageDataCommon::updateCreateLobs( UINT32 createLobs )
   {
      if ( _dmsHeader && _dmsHeader->_createLobs != createLobs )
      {
         _dmsHeader->_createLobs = createLobs ;
         _onHeaderUpdated() ;

         /// flush to file
         flushHeader( isSyncDeep() ) ;
      }
   }
   OSS_INLINE INT32 _dmsStorageDataCommon::getMBContext( dmsMBContext ** pContext,
                                                         UINT16 mbID,
                                                         UINT32 clLID,
                                                         UINT32 startLID,
                                                         INT32 lockType )
   {
      BOOLEAN isInUsed = FALSE ;

      if ( mbID >= DMS_MME_SLOTS )
      {
         return SDB_INVALIDARG ;
      }

      // metadata shared lock
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
         isInUsed = DMS_IS_MB_INUSE( _dmsMME->_mbList[ mbID ]._flag ) ;
         _metadataLatch.release_shared() ;
      }
      else
      {
         isInUsed = DMS_IS_MB_INUSE( _dmsMME->_mbList[ mbID ]._flag ) ;
      }

      if ( !isInUsed )
      {
         return SDB_DMS_NOTEXIST ;
      }

      // context lock
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
      if ( _service )
      {
         pmdEDUCB *cb = pmdGetThreadEDUCB() ;
         dmsCLMetadataKey key( (*pContext)->_mb ) ;
         INT32 rc = _service->getCollection( key, cb, (*pContext)->_collPtr ) ;
         if ( SDB_DMS_NOTEXIST == rc )
         {
            dmsCLMetadata metadata( _suDescriptor,
                                    (*pContext)->_mb,
                                    (*pContext)->_mbStat ) ;
            rc = _service->loadCollection( metadata, cb, (*pContext)->_collPtr ) ;
         }
         if ( rc )
         {
            releaseMBContext( *pContext ) ;
            return rc ;
         }
      }
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

   OSS_INLINE INT32 _dmsStorageDataCommon::getMBContext( dmsMBContext **pContext,
                                                         const CHAR* pName,
                                                         INT32 lockType )
   {
      UINT16 mbID = DMS_INVALID_MBID ;
      UINT32 clLID = DMS_INVALID_CLID ;
      UINT32 startLID = DMS_INVALID_CLID ;

      // metadata shared lock
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

   OSS_INLINE INT32 _dmsStorageDataCommon::getMBContextByID( dmsMBContext **pContext,
                                                             utilCLUniqueID clUniqueID,
                                                             INT32 lockType )
   {
      UINT16 mbID = DMS_INVALID_MBID ;
      UINT32 clLID = DMS_INVALID_CLID ;
      UINT32 startLID = DMS_INVALID_CLID ;

      // metadata shared lock
      _metadataLatch.get_shared() ;
      mbID = _collectionIdLookup( clUniqueID ) ;
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
         try
         {
            _vecContext.push_back( pContext ) ;
         }
         catch( ... )
         {
            SDB_OSS_DEL pContext ;
         }
      }
      else
      {
         SDB_OSS_DEL pContext ;
      }
      _latchContext.release() ;
      pContext = NULL ;
   }

   OSS_INLINE dmsMBStatInfo* _dmsStorageDataCommon::getMBStatInfo( UINT16 mbID )
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

   OSS_INLINE INT32 _dmsStorageDataCommon::getMBInfo( const CHAR *pName,
                                                      UINT16 &mbID,
                                                      UINT32 &clLID,
                                                      utilCLUniqueID &clUniqueID )
   {
      INT32 rc = SDB_OK ;

      clUniqueID = UTIL_UNIQUEID_NULL ;

      if ( NULL == pName )
      {
         rc = SDB_INVALIDARG ;
      }
      else
      {
         // metadata shared lock
         ossScopedLock lock( &_metadataLatch, SHARED ) ;
         mbID = _collectionNameLookup( pName ) ;
         if ( DMS_INVALID_MBID != mbID )
         {
            clLID = _dmsMME->_mbList[ mbID ]._logicalID ;
            clUniqueID = _dmsMME->_mbList[ mbID ]._clUniqueID ;
         }
         else
         {
            rc = SDB_DMS_NOTEXIST ;
         }
      }

      return rc ;
   }

   /*
      Tool Functions
   */
   BOOLEAN  dmsIsRecordIDValid( const BSONElement &oidEle,
                                BOOLEAN allowEOO,
                                const CHAR **pErrStr = NULL ) ;

   /// crud statistics function
#define DMS_MBSTAT_ONCE_INC( _monAppCB_, _mbContext_, op, delta )             \
   {                                                                          \
      if ( NULL != _monAppCB_ && NULL != _mbContext_ )                        \
      {                                                                       \
         _mbContext_->mbStat()->_crudCB.increaseOnce( ( op ), ( delta ) ) ;   \
      }                                                                       \
   }

#define DMS_MBSTAT_INC( _monAppCB_, _mbContext_, op, delta )                  \
   {                                                                          \
      if ( NULL != _monAppCB_ && NULL != _mbContext_ )                        \
      {                                                                       \
         _mbContext_->mbStat()->_crudCB.increase( ( op ), ( delta ) ) ;       \
      }                                                                       \
   }

}

#endif /* DMSSTORAGE_DATACOMMON_HPP_ */

