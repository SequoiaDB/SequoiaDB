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

   Source File Name = monDMS.hpp

   Descriptive Name = Monitor Data Management Service Header

   When/how to use: this program may be used on binary and text-formatted
   versions of monitoring component. This file contains structure for
   DMS information.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MONDMS_HPP_
#define MONDMS_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "dms.hpp"
#include "ixm.hpp"
#include "ixmUtil.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.h"
#include "../bson/bsonobj.h"
#include <set>
#include <vector>
#include "ossMemPool.hpp"
#include "monCB.hpp"
#include "utilRecycleItem.hpp"
#include "dmsStatUnit.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _monIndex define
   */
   class _monIndex : public SDBObject
   {
   public:
      UINT16         _indexFlag ;
      CHAR           _version ;
      dmsRecordID    _scanRID ;
      dmsExtentID    _indexLID ;
      BSONObj        _indexDef ;
      CHAR           _extDataName[ DMS_MAX_EXT_NAME_SIZE + 1 ] ;

      _monIndex()
      {
         _indexFlag = 0 ;
         _version = 0 ;
         _scanRID.reset() ;
         _indexLID = -1 ;
         ossMemset( _extDataName, 0, DMS_MAX_EXT_NAME_SIZE + 1 ) ;
      }

      OSS_INLINE const CHAR *getIndexName () const
      {
         return _indexDef.getStringField( IXM_NAME_FIELD ) ;
      }

      OSS_INLINE BSONObj getKeyPattern () const
      {
         return _indexDef.getObjectField( IXM_KEY_FIELD ) ;
      }

      OSS_INLINE BOOLEAN isUnique () const
      {
         return _indexDef.getBoolField( IXM_UNIQUE_FIELD ) ;
      }

      OSS_INLINE INT32 getIndexType( UINT16 &type ) const
      {
         return ixmGetIndexType( _indexDef, type ) ;
      }

      OSS_INLINE const CHAR* getExtDataName() const
      {
         return _indexDef.getStringField( FIELD_NAME_EXT_DATA_NAME ) ;
      }
   } ;

   typedef class _monIndex monIndex ;
   typedef vector<monIndex> MON_IDX_LIST ;

   /*
      _detailedInfo define
   */
   class _detailedInfo : public SDBObject
   {
   public :
      UINT32 _numIndexes ;
      UINT16 _blockID ;
      UINT16 _flag ;
      UINT32 _logicID ;

      UINT32 _attribute ;
      UINT32 _dictCreated ;
      UINT8  _compressType ;
      UINT8  _dictVersion ;

      UINT32 _pageSize ;
      UINT32 _lobPageSize ;

      // stat info
      UINT64 _totalRecords ;
      UINT64 _totalLobs ;
      UINT32 _totalDataPages ;
      UINT32 _totalIndexPages ;
      UINT32 _totalLobPages ;
      UINT64 _totalUsedLobSpace ;
      FLOAT64 _usedLobSpaceRatio ;
      UINT64 _totalLobSize ;
      UINT64 _totalValidLobSize ;
      FLOAT64 _lobUsageRate ;
      UINT64 _avgLobSize ;
      UINT64 _totalDataFreeSpace ;
      UINT64 _totalIndexFreeSpace ;
      UINT32 _currCompressRatio ;
      // end

      /// sync info
      UINT64 _dataCommitLSN ;
      UINT64 _idxCommitLSN ;
      UINT64 _lobCommitLSN ;
      UINT64 _maxGlobTransID ;
      BOOLEAN _dataIsValid ;
      BOOLEAN _idxIsValid ;
      BOOLEAN _lobIsValid ;

      UINT64  _createTime ;
      UINT64  _updateTime ;

      monCRUDCB _crudCB ;

      _detailedInfo ()
      : _crudCB()
      {
         _numIndexes          = 0 ;
         _blockID             = 0 ;
         _flag                = 0 ;
         _flag                = 0 ;
         _logicID             = 0 ;

         _attribute           = 0 ;
         _dictCreated         = FALSE ;
         _compressType        = 0 ;
         _dictVersion         = 0 ;

         _pageSize            = 0 ;
         _lobPageSize         = 0 ;

         _totalRecords        = 0 ;
         _totalLobs           = 0 ;
         _totalDataPages      = 0 ;
         _totalIndexPages     = 0 ;
         _totalLobPages       = 0 ;
         _totalUsedLobSpace   = 0 ;
         _usedLobSpaceRatio   = 0 ;
         _totalLobSize        = 0 ;
         _totalValidLobSize   = 0 ;
         _lobUsageRate        = 0 ;
         _avgLobSize          = 0 ;
         _totalDataFreeSpace  = 0 ;
         _totalIndexFreeSpace = 0 ;
         _currCompressRatio   = 0 ;

         /// sync info
         _dataCommitLSN       = -1 ;
         _idxCommitLSN        = -1 ;
         _lobCommitLSN        = -1 ;
         _maxGlobTransID      = 0 ;
         _dataIsValid         = FALSE ;
         _idxIsValid          = FALSE ;
         _lobIsValid          = FALSE ;

         _createTime          = 0 ;
         _updateTime          = 0 ;
      }
   } ;
   typedef class _detailedInfo detailedInfo ;
   typedef ossPoolMap<UINT32, detailedInfo>  MON_CL_DETAIL_MAP ;

   /*
      _collectionStatInfo define
   */
   class _collectionStatInfo : public SDBObject
   {
   public:
      CHAR           _collection[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
      BOOLEAN        _isDefault ;
      UINT32         _isExpired ;
      CHAR           _statTimestamp[ OSS_TIMESTAMP_STRING_LEN + 1 ] ;
      UINT64         _avgNumFields ;
      UINT64         _sampleRecords ;
      UINT64         _totalRecords ;
      UINT64         _totalDataSize ;
      UINT64         _totalDataPages ;

      _collectionStatInfo()
      {
         reset() ;
      }

      BOOLEAN inited() const
      {
         return ( _collection[0] != 0 ) ;
      }

      void reset()
      {
         ossMemset( _collection, 0, sizeof( _collection ) ) ;
         ossMemset( _statTimestamp, 0, sizeof( _statTimestamp ) ) ;
         _isDefault = TRUE ;
         _isExpired = FALSE ;
         _avgNumFields = DMS_STAT_DEF_AVG_NUM_FIELDS ;
         _sampleRecords = DMS_STAT_DEF_TOTAL_RECORDS ;
         _totalRecords = DMS_STAT_DEF_TOTAL_RECORDS ;
         _totalDataPages = DMS_STAT_DEF_TOTAL_PAGES ;
         _totalDataSize = DMS_STAT_DEF_TOTAL_RECORDS * DMS_STAT_DEF_DATA_SIZE ;
      }

      void setCollectionName( const CHAR *collectionName )
      {
         ossStrncpy( _collection, collectionName, sizeof( _collection ) - 1 ) ;
      }

   } ;

   typedef class _collectionStatInfo collectionStatInfo ;

   /*
      _monCLSimple define
   */
   class _monCLSimple : public SDBObject
   {
      public:
         CHAR  _name[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;

         CHAR _csname [ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;
         CHAR _clname [ DMS_COLLECTION_NAME_SZ + 1 ] ;

         UINT16 _blockID ;
         UINT32 _logicalID ;
         utilCLUniqueID _clUniqueID ;

         MON_IDX_LIST _idxList ;

         _monCLSimple ()
         {
            _name[ 0 ] = 0 ;
            _clname[ 0 ] = 0 ;
            _csname[ 0 ] = 0 ;
            _blockID = 0 ;
            _logicalID = 0 ;
            _clUniqueID = UTIL_UNIQUEID_NULL ;
         }

         BOOLEAN operator< (const _monCLSimple &r) const
         {
            return ossStrncmp( _name, r._name, sizeof( _name ) ) < 0 ;
         }

         OSS_INLINE void setName ( const CHAR *pCSName, const CHAR *pCLName )
         {
            ossMemset( _name, 0, sizeof( _name ) ) ;
            ossStrncpy( _name, pCSName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
            ossStrncat( _name, ".", 1 ) ;
            ossStrncat( _name, pCLName, DMS_COLLECTION_NAME_SZ ) ;

            ossMemset( _csname, 0, sizeof( _csname ) ) ;
            ossStrncpy( _csname, pCSName, DMS_COLLECTION_SPACE_NAME_SZ ) ;

            ossMemset( _clname, 0, sizeof( _clname ) ) ;
            ossStrncpy( _clname, pCLName, DMS_COLLECTION_NAME_SZ ) ;
         }

         const monIndex *getIndex ( const CHAR *pIndexName ) const
         {
            MON_IDX_LIST::const_iterator iterIdx = _idxList.begin() ;
            while ( iterIdx != _idxList.end() )
            {
               if ( 0 == ossStrcmp( iterIdx->getIndexName(), pIndexName ) )
               {
                  return &(*iterIdx) ;
               }
               ++ iterIdx ;
            }
            return NULL ;
         }
   } ;

   typedef class _monCLSimple monCLSimple ;

   typedef ossPoolSet<monCLSimple>  MON_CL_SIM_LIST ;
   typedef std::vector<monCLSimple> MON_CL_SIM_VEC ;

   /*
      _monCollection define
   */
   class _monCollection : public SDBObject
   {
   public :
      CHAR _name [ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
      utilCLUniqueID _clUniqueID ;
      MON_CL_DETAIL_MAP _details ;

      _monCollection()
      {
         _name[ 0 ]  = 0 ;
         _clUniqueID = UTIL_UNIQUEID_NULL ;
      }
      OSS_INLINE BOOLEAN operator<(const _monCollection &r) const
      {
         return ossStrncmp( _name, r._name, sizeof(_name))<0 ;
      }
      OSS_INLINE detailedInfo& addDetails ( UINT32 sequence, UINT32 numIndexes,
                                            UINT16 blockID, UINT16 flag,
                                            UINT32 logicID, UINT64 totalRecords,
                                            UINT32 totalDataPages,
                                            UINT32 totalIndexPages,
                                            UINT32 totalLobPages,
                                            UINT64 totalDataFreeSpace,
                                            UINT64 totalIndexFreeSpace )
      {
         detailedInfo &info = _details[ sequence ] ;
         info._numIndexes = numIndexes ;
         info._blockID = blockID ;
         info._flag = flag ;
         info._logicID = logicID ;

         info._totalRecords        = totalRecords ;
         info._totalDataPages      = totalDataPages ;
         info._totalIndexPages     = totalIndexPages ;
         info._totalLobPages       = totalLobPages ;
         info._totalDataFreeSpace  = totalDataFreeSpace ;
         info._totalIndexFreeSpace = totalIndexFreeSpace ;

         return info ;
      }

      OSS_INLINE void setName ( const CHAR *pCSName, const CHAR *pCLName )
      {
         ossMemset( _name, 0, sizeof( _name ) ) ;
         ossStrncpy( _name, pCSName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
         ossStrncat( _name, ".", 1 ) ;
         ossStrncat( _name, pCLName, DMS_COLLECTION_NAME_SZ ) ;
      }

   } ;
   typedef class _monCollection        monCollection ;
   typedef ossPoolSet<monCollection>   MON_CL_LIST ;

   /*
      _monCSSimple define
   */

   class _monCSSimple ;
   typedef class _monCSSimple       monCSSimple ;
   typedef ossPoolSet<monCSSimple>  MON_CS_SIM_LIST ;

   class _monCSSimple : public SDBObject
   {
      public:
         CHAR  _name[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;
         dmsStorageUnitID _suID ;
         UINT32 _logicalID ;
         MON_CL_SIM_VEC _clList ;
         utilCSUniqueID _csUniqueID ;

         _monCSSimple ()
         {
            _name[ 0 ] = 0 ;
            _suID = DMS_INVALID_SUID ;
            _logicalID = DMS_INVALID_LOGICCSID ;
            _csUniqueID = UTIL_UNIQUEID_NULL ;
         }

         BOOLEAN operator< ( const _monCSSimple &r ) const
         {
            return ossStrncmp( _name, r._name, sizeof( _name ) ) < 0 ;
         }

         OSS_INLINE void setName ( const CHAR *pCSName )
         {
            ossMemset ( _name, 0, sizeof( _name ) ) ;
            ossStrncpy ( _name, pCSName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
         }

         const monCLSimple *getCollection ( const CHAR *pCLName ) const
         {
            MON_CL_SIM_VEC::const_iterator iterCL = _clList.begin() ;
            while ( iterCL != _clList.end() )
            {
               if ( 0 == ossStrncmp( iterCL->_clname, pCLName,
                                     DMS_COLLECTION_NAME_SZ ) )
               {
                  return &(*iterCL) ;
               }
               ++ iterCL ;
            }
            return NULL ;
         }

         static const monCSSimple *getCollectionSpace ( const MON_CS_SIM_LIST &monCSList,
                                                        const CHAR *pCSName )
         {
            MON_CS_SIM_LIST::const_iterator iterCS = monCSList.begin() ;
            while ( iterCS != monCSList.end() )
            {
               if ( 0 == ossStrncmp( iterCS->_name, pCSName,
                                     DMS_COLLECTION_SPACE_NAME_SZ ) )
               {
                  return &(*iterCS) ;
               }
               ++ iterCS ;
            }
            return NULL ;
         }
   } ;

   /*
      _monCollectionSpace define
   */
   class _monCollectionSpace : public SDBObject
   {
   public :
      CHAR _name [ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;
      utilCSUniqueID _csUniqueID ;
      INT32          _suID ;
      UINT32         _csLID ;
      MON_CL_SIM_VEC _collections ;
      INT32 _pageSize ;
      INT32 _clNum ;
      INT64 _totalRecordNum ;
      INT64 _totalSize ;
      INT64 _freeSize ;
      INT32 _lobPageSize ;
      INT64 _totalDataSize ;
      INT64 _totalIndexSize ;
      INT64 _totalLobPages ;
      INT64 _totalLobs ;
      INT64 _totalValidLobSize ;
      INT64 _lobCapacity ;
      INT64 _lobMetaCapacity ;
      INT64 _totalLobSize ;
      INT64 _freeDataSize ;
      INT64 _freeIndexSize ;
      INT64 _freeLobSpace ;
      INT64 _totalLobGet ;
      INT64 _totalLobPut ;
      INT64 _totalLobDelete ;
      INT64 _totalLobReadSize ;
      INT64 _totalLobWriteSize ;
      INT64 _totalLobRead ;
      INT64 _totalLobWrite ;
      INT64 _totalLobTruncate ;
      INT64 _totalLobAddressing ;
      INT64 _totalLobList ;
      INT64 _recycleDataSize ;
      INT64 _recycleIndexSize ;
      INT64 _recycleLobSize ;

      /// commit info
      UINT64 _dataCommitLsn ;
      UINT64 _idxCommitLsn ;
      UINT64 _lobCommitLsn ;
      BOOLEAN _dataIsValid ;
      BOOLEAN _idxIsValid ;
      BOOLEAN _lobIsValid ;

      /// cache info
      UINT32 _dirtyPage ;
      DMS_STORAGE_TYPE _type ;

      UINT64 _createTime ;
      UINT64 _updateTime ;

      _monCollectionSpace ()
      {
         ossMemset ( _name, 0, sizeof(_name)) ;
         _csUniqueID = UTIL_UNIQUEID_NULL ;
         _suID = -1 ;
         _csLID = 0 ;
         _pageSize = 0 ;
         _clNum    = 0 ;
         _totalRecordNum = 0 ;
         _totalSize = 0 ;
         _freeSize  = 0 ;
         _lobPageSize = 0 ;
         _totalDataSize = 0 ;
         _totalIndexSize = 0 ;
         _totalLobPages = 0 ;
         _totalLobs = 0 ;
         _totalValidLobSize = 0 ;
         _lobCapacity = 0 ;
         _lobMetaCapacity = 0 ;
         _totalLobSize = 0 ;
         _freeDataSize = 0 ;
         _freeIndexSize = 0 ;
         _freeLobSpace = 0 ;
         _totalLobGet = 0 ;
         _totalLobPut = 0 ;
         _totalLobDelete = 0 ;
         _totalLobReadSize = 0 ;
         _totalLobWriteSize = 0 ;
         _totalLobRead = 0 ;
         _totalLobWrite = 0 ;
         _totalLobTruncate = 0 ;
         _totalLobAddressing = 0 ;
         _totalLobList = 0 ;
         _recycleDataSize = 0 ;
         _recycleIndexSize = 0 ;
         _recycleLobSize = 0 ;

         _dataCommitLsn = -1 ;
         _idxCommitLsn = -1 ;
         _lobCommitLsn = -1 ;
         _dataIsValid = FALSE ;
         _idxIsValid = FALSE ;
         _lobIsValid = FALSE ;

         _dirtyPage = 0 ;
         _type = DMS_STORAGE_NORMAL ;

         _createTime = 0 ;
         _updateTime = 0 ;
      }
      _monCollectionSpace ( const _monCollectionSpace &right )
      {
         ossStrcpy ( _name, right._name ) ;
         _csUniqueID = right._csUniqueID ;
         _collections = right._collections ;
         _suID = right._suID ;
         _csLID = right._csLID ;
         _pageSize = right._pageSize ;
         _clNum    = right._clNum ;
         _totalRecordNum = right._totalRecordNum ;
         _totalSize = right._totalSize ;
         _freeSize  = right._freeSize ;
         _lobPageSize = right._lobPageSize ;
         _totalDataSize = right._totalDataSize ;
         _totalIndexSize = right._totalIndexSize ;
         _totalLobPages = right._totalLobPages ;
         _totalLobs = right._totalLobs ;
         _totalValidLobSize = right._totalValidLobSize ;
         _lobCapacity = right._lobCapacity ;
         _lobMetaCapacity = right._lobMetaCapacity ;
         _totalLobSize = right._totalLobSize ;
         _freeDataSize = right._freeDataSize ;
         _freeIndexSize = right._freeIndexSize ;
         _freeLobSpace = right._freeLobSpace ;
         _totalLobGet = right._totalLobGet ;
         _totalLobPut = right._totalLobPut ;
         _totalLobDelete = right._totalLobDelete ;
         _totalLobReadSize = right._totalLobReadSize ;
         _totalLobWriteSize = right._totalLobWriteSize ;
         _totalLobRead = right._totalLobRead ;
         _totalLobWrite = right._totalLobWrite ;
         _totalLobTruncate = right._totalLobTruncate ;
         _totalLobAddressing = right._totalLobAddressing ;
         _totalLobList = right._totalLobList ;
         _recycleDataSize = right._recycleDataSize ;
         _recycleIndexSize = right._recycleIndexSize ;
         _recycleLobSize = right._recycleLobSize ;

         _dataCommitLsn = right._dataCommitLsn ;
         _idxCommitLsn = right._idxCommitLsn ;
         _lobCommitLsn = right._lobCommitLsn ;
         _dataIsValid = right._dataIsValid ;
         _idxIsValid = right._idxIsValid ;
         _lobIsValid = right._lobIsValid ;

         _dirtyPage = right._dirtyPage ;
         _type = right._type ;

         _createTime = right._createTime ;
         _updateTime = right._updateTime ;
      }
      ~_monCollectionSpace()
      {
         _collections.clear() ;
      }

      OSS_INLINE BOOLEAN operator<(const _monCollectionSpace &r) const
      {
         return ossStrncmp( _name, r._name, sizeof(_name))<0 ;
      }
      _monCollectionSpace &operator= (const _monCollectionSpace &right)
      {
         ossStrcpy ( _name, right._name ) ;
         _csUniqueID = right._csUniqueID ;
         _collections = right._collections ;
         _suID = right._suID ;
         _csLID = right._csLID ;
         _pageSize = right._pageSize ;
         _clNum    = right._clNum ;
         _totalRecordNum = right._totalRecordNum ;
         _totalSize      = right._totalSize ;
         _freeSize       = right._freeSize ;
         _lobPageSize    = right._lobPageSize ;
         _totalDataSize = right._totalDataSize ;
         _totalIndexSize = right._totalIndexSize ;
         _totalLobPages = right._totalLobPages ;
         _totalLobs = right._totalLobs ;
         _totalValidLobSize = right._totalValidLobSize ;
         _lobCapacity = right._lobCapacity ;
         _lobMetaCapacity = right._lobMetaCapacity ;
         _totalLobSize = right._totalLobSize ;
         _freeDataSize = right._freeDataSize ;
         _freeIndexSize = right._freeIndexSize ;
         _freeLobSpace = right._freeLobSpace ;
         _totalLobGet = right._totalLobGet ;
         _totalLobPut = right._totalLobPut ;
         _totalLobDelete = right._totalLobDelete ;
         _totalLobReadSize = right._totalLobReadSize ;
         _totalLobWriteSize = right._totalLobWriteSize ;
         _totalLobRead = right._totalLobRead ;
         _totalLobWrite = right._totalLobWrite ;
         _totalLobTruncate = right._totalLobTruncate ;
         _totalLobAddressing = right._totalLobAddressing ;
         _totalLobList = right._totalLobList ;
         _recycleDataSize = right._recycleDataSize ;
         _recycleIndexSize = right._recycleIndexSize ;
         _recycleLobSize = right._recycleLobSize ;

         _dataCommitLsn = right._dataCommitLsn ;
         _idxCommitLsn = right._idxCommitLsn ;
         _lobCommitLsn = right._lobCommitLsn ;
         _dataIsValid = right._dataIsValid ;
         _idxIsValid = right._idxIsValid ;
         _lobIsValid = right._lobIsValid ;

         _dirtyPage = right._dirtyPage ;
         _type = right._type ;

         _createTime = right._createTime ;
         _updateTime = right._updateTime ;

         return *this ;
      }
   } ;
   typedef class _monCollectionSpace      monCollectionSpace ;
   typedef ossPoolSet<monCollectionSpace> MON_CS_LIST ;

   /*
      _monStorageUnit define
   */
   class _monStorageUnit : public SDBObject
   {
   public :
      CHAR _name [ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;
      utilCSUniqueID _csUniqueID ;
      dmsStorageUnitID _CSID ;
      UINT32 _logicalCSID ;
      SINT32 _pageSize ;
      SINT32 _lobPageSize ;
      SINT32 _sequence ;
      SINT32 _numCollections ;
      SINT32 _collectionHWM ;
      SINT64 _size ;
      UINT64 _createTime ;
      UINT64 _updateTime ;

      OSS_INLINE BOOLEAN operator<(const _monStorageUnit &r) const
      {
         SINT32 rc = ossStrncmp( _name, r._name, sizeof(_name))<0 ;
         // if two storage unit got same name, let's check sequence
         if ( !rc )
            return _sequence < r._sequence ;
         return rc ;
      }

      OSS_INLINE void setName ( const CHAR *pCSName )
      {
         ossMemset ( _name, 0, sizeof( _name ) ) ;
         ossStrncpy ( _name, pCSName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
      }

      _monStorageUnit()
      {
         _name[ 0 ] = 0 ;
         _csUniqueID = UTIL_UNIQUEID_NULL ;
         _CSID = -1 ;
         _logicalCSID = 0 ;
         _pageSize = 0 ;
         _lobPageSize = 0 ;
         _sequence = 0 ;
         _numCollections = 0 ;
         _collectionHWM = 0 ;
         _size = 0 ;
         _createTime = 0 ;
         _updateTime = 0 ;
      }
   } ;
   typedef class _monStorageUnit       monStorageUnit ;
   typedef ossPoolSet<monStorageUnit>  MON_SU_LIST ;

   /*
      _monCSName define
   */
   struct _monCSName
   {
      CHAR     _csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;

      _monCSName( const CHAR *pCSName = NULL )
      {
         ossMemset( _csName, 0, sizeof( _csName ) ) ;

         if ( pCSName )
         {
            ossStrncpy( _csName, pCSName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
         }
      }

      _monCSName( const _monCSName &right )
      {
         ossStrcpy( _csName, right._csName ) ;
      }

      _monCSName& operator= ( const _monCSName &right )
      {
         ossStrcpy( _csName, right._csName ) ;
         return *this ;
      }
   } ;
   typedef _monCSName monCSName ;
   typedef std::vector< monCSName >          MON_CSNAME_VEC ;

   class _monRecycleItem : public _utilRecycleItem
   {
   public:
      _monRecycleItem()
      : _utilRecycleItem(),
        _pageSize( 0 ),
        _lobPageSize( 0 ),
        _totalRecords( 0 ),
        _totalLobs( 0 ),
        _totalDataSize( 0 ),
        _totalIndexSize( 0 ),
        _totalLobSize( 0 )
      {
      }

      _monRecycleItem( const _utilRecycleItem &item )
      : _utilRecycleItem( item ),
        _pageSize( 0 ),
        _lobPageSize( 0 ),
        _totalRecords( 0 ),
        _totalLobs( 0 ),
        _totalDataSize( 0 ),
        _totalIndexSize( 0 ),
        _totalLobSize( 0 )
      {
      }

      _monRecycleItem( const _monRecycleItem &item )
      : _utilRecycleItem( item ),
        _pageSize( item._pageSize ),
        _lobPageSize( item._lobPageSize ),
        _totalRecords( item._totalRecords ),
        _totalLobs( item._totalLobs ),
        _totalDataSize( item._totalDataSize ),
        _totalIndexSize( item._totalIndexSize ),
        _totalLobSize( item._totalLobSize )
      {
      }

      ~_monRecycleItem()
      {
      }

      _monRecycleItem &operator =( const _monRecycleItem &item )
      {
         _utilRecycleItem::operator =( item ) ;
         _pageSize = item._pageSize ;
         _lobPageSize = item._lobPageSize ;
         _totalRecords = item._totalRecords ;
         _totalLobs = item._totalLobs ;
         _totalDataSize = item._totalDataSize ;
         _totalIndexSize = item._totalIndexSize ;
         _totalLobSize = item._totalLobSize ;

         return ( *this ) ;
      }

   public:
      INT32 _pageSize ;
      INT32 _lobPageSize ;
      INT64 _totalRecords ;
      INT64 _totalLobs ;
      INT64 _totalDataSize ;
      INT64 _totalIndexSize ;
      INT64 _totalLobSize ;
   } ;

   typedef class _monRecycleItem monRecycleItem ;
   typedef ossPoolList< monRecycleItem > MON_RECYCLE_LIST ;

}

#endif //MONDMS_HPP_

