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

   Source File Name = dmsRecord.hpp

   Descriptive Name = Data Management Service Record Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   data record, including normal record and deleted record.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSRECORD_HPP_
#define DMSRECORD_HPP_

#include "dms.hpp"
#include "../bson/bson.h"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "utilCompressor.hpp"
#include "dpsDef.hpp"

namespace engine
{
   // 2^24 = 16MB, shouldn't be changed without completely redesign the
   // dmsRecord structure this size includes metadata header

   #define DMS_RECORD_MAX_SZ           0x01000000

   // since DPS may need extra space for header, let's make sure the max size of
   // user record ( the ones need to log ) cannot exceed 16M-4K

   #define DMS_RECORD_USER_MAX_SZ      (DMS_RECORD_MAX_SZ-4096)

   /*
      _dmsRecordData define
   */
   class _dmsRecordData : public SDBObject
   {
      public:
         _dmsRecordData()
         : _data( NULL ), _len( 0 ), _ownedBuffer( NULL ), _ownedBufferLen( 0 )
         {
         }
         _dmsRecordData( const CHAR *data, UINT32 len )
         : _data( data ), _len( len ), _ownedBuffer( NULL ), _ownedBufferLen( 0 )
         {
         }
         ~_dmsRecordData()
         {
            if ( _ownedBuffer )
            {
               SDB_THREAD_FREE( _ownedBuffer ) ;
               _ownedBuffer = NULL ;
               _ownedBufferLen = 0 ;
            }
         }

         BOOLEAN isEmpty() const { return !_data || 0 == _len ; }

         const CHAR* data() const { return _data ; }
         UINT32 len() const { return _len ; }

         void setData( const CHAR *data, UINT32 len )
         {
            _data = data ;
            _len = len ;
         }

         void reset()
         {
            _data = NULL ;
            _len = 0 ;
         }

         INT32 getOwned()
         {
            if ( _ownedBufferLen < _len )
            {
               CHAR *tmpBuf = (CHAR *)SDB_THREAD_REALLOC( _ownedBuffer, _len ) ;
               if ( NULL == tmpBuf )
               {
                  return SDB_OOM ;
               }
               _ownedBuffer = tmpBuf ;
               _ownedBufferLen = _len ;
            }
            ossMemcpy( _ownedBuffer, _data, _len ) ;
            _data = _ownedBuffer ;
            return SDB_OK ;
         }

      private:
         const CHAR     *_data ;
         UINT32         _len ;
         CHAR           *_ownedBuffer ;
         UINT32         _ownedBufferLen ;
   } ;
   typedef _dmsRecordData dmsRecordData ;

   /*
      Record Flag define:
   */
   // 0~3 bit for STATE
   #define DMS_RECORD_FLAG_NORMAL            0x00
   #define DMS_RECORD_FLAG_OVERFLOWF         0x01 // Overflow from record
   #define DMS_RECORD_FLAG_OVERFLOWT         0x02 // Overflow to record
   #define DMS_RECORD_FLAG_DELETED           0x04
   // 4~7 bit for ATTR
   #define DMS_RECORD_FLAG_COMPRESSED        0x10
   // Indicate this record has global transaction ID, introduced in v1
   #define DMS_RECORD_FLAG_HASGLOBTRANSID    0x20
   // some one wait X-lock, the last one who get X-lock will delete the record
   #define DMS_RECORD_FLAG_DELETING          0x80

   #define DMS_RECORD_V0_METADATA_SZ   sizeof(_dmsRecord_v0)
   #define DMS_RECORD_V1_METADATA_SZ   sizeof(_dmsRecord_v1)
   #define DMS_RECORD_RBS_METADATA_SZ   sizeof(_dmsRBSRecord)
   #define DMS_RECORD_CAP_METADATA_SZ   sizeof(_dmsCappedRecord)
   #define DMS_RECORD_METADATA_SZ DMS_RECORD_V0_METADATA_SZ
   // based on current record version to decide the record metadata size
   #define DMS_RECORD_VERSIONED_METADATA_SZ               \
           ( hasGlobTransID() ? DMS_RECORD_V1_METADATA_SZ : \
                              DMS_RECORD_V0_METADATA_SZ )

   /*
      _dmsRecord defined
   */
   class _dmsRecord_v0 : public SDBObject
   {
   public:
      union
      {
         CHAR     _recordHead[4] ;     // 1 byte flag, 3 bytes length - 1
         UINT32   _flag_and_size ;
      }                 _head ;
      dmsOffset         _myOffset ;
      dmsOffset         _previousOffset ;
      dmsOffset         _nextOffset ;
      /*
         Follow _nextOffset is:
            if overflow, is overflow rid(8bytes)
            else, is the data length(4 bytes).
                In compressed: 4bytes + data
                uncompressed:  data( the first 4bytes is bson size )
      */

      /*
         Get Functions
      */
      dmsOffset getMyOffset() const
      {
         return _myOffset ;
      }
      dmsOffset getPrevOffset() const
      {
         return _previousOffset ;
      }
      dmsOffset getNextOffset() const
      {
         return _nextOffset ;
      }
      BYTE getFlag() const
      {
         return (BYTE)_head._recordHead[ 0 ] ;
      }
      BYTE getState() const
      {
         return (BYTE)(getFlag() & 0x0F) ;
      }
      BYTE getAttr() const
      {
         return (BYTE)(getFlag() & 0xF0) ;
      }
      BOOLEAN isOvf() const
      {
         return  DMS_RECORD_FLAG_OVERFLOWF == getState() ;
      }
      BOOLEAN isOvt() const
      {
         return DMS_RECORD_FLAG_OVERFLOWT == getState() ;
      }
      BOOLEAN isDeleted() const
      {
         return DMS_RECORD_FLAG_DELETED == getState() ;
      }
      BOOLEAN isNormal() const
      {
         return DMS_RECORD_FLAG_NORMAL == getState() ;
      }
      BOOLEAN isCompressed() const
      {
         return getAttr() & DMS_RECORD_FLAG_COMPRESSED ;
      }
      BOOLEAN isDeleting() const
      {
         return getAttr() & DMS_RECORD_FLAG_DELETING ;
      }
      BOOLEAN hasGlobTransID() const
      {
         return getAttr() & DMS_RECORD_FLAG_HASGLOBTRANSID ;
      }
      OSS_INLINE dmsRecordID getOvfRID() const ;

      UINT32 getSize() const
      {
#if defined (SDB_BIG_ENDIAN)
         return (((*((const UINT32*)this))&0x00FFFFFF)+1) ;
#else
         return (((*((const UINT32*)this))>>8)+1) ;
#endif // SDB_BIG_ENDIAN
      }

      OSS_INLINE UINT32 getHeaderSize() const ;

      OSS_INLINE UINT8 getCompressType () const ;

      OSS_INLINE UINT32 getDataLength() const ;
      /*
         Get disk data only, if compressed, not uncompressed
      */
      OSS_INLINE const CHAR* getData() const ;

      /*
         Set Functions
      */
      void setMyOffset( dmsOffset offset )
      {
         _myOffset = offset ;
      }
      void setPrevOffset( dmsOffset offset )
      {
         _previousOffset = offset ;
      }
      void setNextOffset( dmsOffset offset )
      {
         _nextOffset = offset ;
      }
      void  setFlag( BYTE flag )
      {
         _head._recordHead[ 0 ] = (CHAR)flag ;
      }
      void  setState( BYTE state )
      {
         _head._recordHead[ 0 ] = (CHAR)((state&0x0F)|getAttr()) ;
      }
      void setAttr( BYTE attr )
      {
         _head._recordHead[ 0 ] |= (BYTE)(attr&0xF0) ;
      }
      void unsetAttr( BYTE attr )
      {
         _head._recordHead[ 0 ] &= (BYTE)(~(attr&0xF0)) ;
      }
      void resetAttr()
      {
         unsetAttr( 0xF0 ) ;
      }
      void setOvf()
      {
         setState( DMS_RECORD_FLAG_OVERFLOWF ) ;
      }
      void setOvt()
      {
         setState( DMS_RECORD_FLAG_OVERFLOWT ) ;
      }
      void setNormal()
      {
         setState( DMS_RECORD_FLAG_NORMAL ) ;
      }
      void setDeleted()
      {
         setState( DMS_RECORD_FLAG_DELETED ) ;
      }
      void setCompressed()
      {
         setAttr( DMS_RECORD_FLAG_COMPRESSED ) ;
      }
      void unsetCompressed()
      {
         unsetAttr( DMS_RECORD_FLAG_COMPRESSED ) ;
      }
      void unsetHasGlobTransID()
      {
         unsetAttr( DMS_RECORD_FLAG_HASGLOBTRANSID ) ;
      }
      void setDeleting()
      {
         setAttr( DMS_RECORD_FLAG_DELETING ) ;
      }
      void unsetDeleting()
      {
         unsetAttr( DMS_RECORD_FLAG_DELETING ) ;
      }
      OSS_INLINE void setOvfRID( const dmsRecordID &rid ) ;

      void  setSize( UINT32 size )
      {
#if defined (SDB_BIG_ENDIAN)
      (*((UINT32*)this) = ((UINT32)getFlag()<<24) |
                          ((UINT32)((size)-1)&0x00FFFFFF)) ;
#else
      (*((UINT32*)this) = (UINT32)getFlag() | ((UINT32)((size)-1)<<8)) ;
#endif
      }

      OSS_INLINE void setCompressType ( UINT8 type ) ;

      /*
         Copy the data to disk directly
      */
      OSS_INLINE void  setData( const dmsRecordData &data ) ;
   } ;
   typedef _dmsRecord_v0 dmsRecord_v0 ;
   

   class _dmsRecord_v1 : public _dmsRecord_v0
   {
   public :
     CHAR _globTransID[ 12 ] ;  // global transaction ID

      /*
         Follow _globTransID is:
            if overflow, is overflow rid(8bytes)
            else, is the data length(4 bytes).
                In compressed: 4bytes + data
                uncompressed:  data( the first 4bytes is bson size )
      */
   };
   typedef _dmsRecord_v1 dmsRecord_v1 ;

   // current version is v1 which has GlobTransID for MVCC purpose
   typedef _dmsRecord_v0 _dmsRecord ;
   typedef _dmsRecord dmsRecord ;

   // implementations has to put after _dmsRecord_v1 definition
   OSS_INLINE dmsRecordID _dmsRecord_v0::getOvfRID() const 
   {
      if ( isOvf() )
      {
         return *( const dmsRecordID* )( (const CHAR*)this +
                                      DMS_RECORD_VERSIONED_METADATA_SZ ) ;
      }
      return dmsRecordID() ;
   }

   OSS_INLINE UINT32 _dmsRecord_v0::getHeaderSize() const
   {
      return DMS_RECORD_VERSIONED_METADATA_SZ ;
   }

   OSS_INLINE UINT8 _dmsRecord_v0::getCompressType () const
   {
#if defined (SDB_BIG_ENDIAN)
      return ( (const UINT8 *)this + DMS_RECORD_VERSIONED_METADATA_SZ )[ 0 ] ;
#else
      return ( (const UINT8 *)this + DMS_RECORD_VERSIONED_METADATA_SZ )[ 3 ] ;
#endif // SDB_BIG_ENDIAN
   }

   OSS_INLINE UINT32 _dmsRecord_v0::getDataLength() const
   {
      return ( *(const UINT32 *)
                ( (const CHAR*)this + DMS_RECORD_VERSIONED_METADATA_SZ ) )
             & 0x00FFFFFF ;
   }

   OSS_INLINE const CHAR* _dmsRecord_v0::getData() const
   {
      return isCompressed() ?
         ((const CHAR*)this+sizeof(UINT32)+ DMS_RECORD_VERSIONED_METADATA_SZ) :
         ((const CHAR*)this+DMS_RECORD_VERSIONED_METADATA_SZ) ;
   }

   OSS_INLINE void _dmsRecord_v0::setOvfRID( const dmsRecordID &rid )
   {
      *((dmsRecordID*)((CHAR*)this+DMS_RECORD_VERSIONED_METADATA_SZ)) = rid ;
   }

   OSS_INLINE void _dmsRecord_v0::setCompressType ( UINT8 type )
   {
      *(UINT32 *)( (CHAR *)this + DMS_RECORD_VERSIONED_METADATA_SZ ) |=
         ( ( (UINT32)type << 24 ) & 0xFF000000 ) ;
//#if defined (SDB_BIG_ENDIAN)
//      ( (UINT8 *)this + DMS_RECORD_VERSIONED_METADATA_SZ )[ 0 ] = type ;
//#else
//      ( (UINT8 *)this + DMS_RECORD_VERSIONED_METADATA_SZ )[ 3 ] = type ;
//#endif // SDB_BIG_ENDIAN
   }

   OSS_INLINE void  _dmsRecord_v0::setData( const dmsRecordData &data )
   {
      if ( data.isEmpty() )
      {
         return ;
      }
      unsetCompressed() ;
      ossMemcpy( (CHAR*)this+DMS_RECORD_VERSIONED_METADATA_SZ,
                 data.data(), data.len() ) ;
   }

   // Extract Data
   #define DMS_RECORD_EXTRACTDATA( pRecord, retPtr, compressorEntry )   \
   do {                                                                 \
         if ( !pRecord->isCompressed() )                                \
         {                                                              \
            (retPtr) = (ossValuePtr)pRecord->getData() ;                \
         }                                                              \
         else                                                           \
         {                                                              \
            INT32 uncompLen = 0 ;                                       \
            UINT8 compressType = pRecord->getCompressType() ;           \
            rc = dmsUncompress( cb, compressorEntry, compressType,      \
                                pRecord->getData(),                     \
                                pRecord->getDataLength(),               \
                                (const CHAR**)&(retPtr), &uncompLen ) ; \
            PD_RC_CHECK ( rc, PDERROR,                                  \
                          "Failed to uncompress record, rc = %d", rc ); \
            PD_CHECK ( uncompLen == *(INT32*)(retPtr),                  \
                       SDB_CORRUPTED_RECORD, error, PDERROR,            \
                       "uncompressed length %d does not match real "    \
                       "len %d", uncompLen, *(INT32*)(retPtr) ) ;       \
         }                                                              \
      } while ( FALSE )

   // Capped collectionr record header.
   class _dmsCappedRecord : public SDBObject
   {
      // Caution: This structure will use dmsRecord structure for data and
      // attribute setting. DO NOT modify the members!
   public:
      union
      {
         CHAR     _recordHead[4] ;
         UINT32   _flag_and_size ;
      }           _head ;
      // Record number which have been inserted into this extent. It's used to
      // calculate record number being popped in pop operation, avoid scanning
      // the whole extent.
      // Note: It includes records which have been popped out forward, but
      // excludes those which have been popped out backward. In a valid record,
      // it should always be greater than 0.
      UINT32      _recNo ;
      // similar to LR LSN, logical ID is an strictly incremental offset of
      // an record within the capped CS
      INT64       _logicalID ;

   public:
      CHAR getFlag() const
      {
         return _head._recordHead[ 0 ] ;
      }

      void setSize( UINT32 size )
      {
         return ((dmsRecord*)this)->setSize( size ) ;
      }

      UINT32 getSize() const
      {
         return ((const dmsRecord*)this)->getSize() ;
      }

      void setLogicalID( INT64 logicalID )
      {
         _logicalID = logicalID ;
      }

      INT64 getLogicalID() const
      {
         return _logicalID ;
      }

      void setRecordNo( UINT32 recNo )
      {
         _recNo = recNo ;
      }

      UINT32 getRecordNo() const
      {
         return _recNo ;
      }

      void setNormal()
      {
         return ((dmsRecord*)this)->setNormal() ;
      }

      BOOLEAN isNormal() const
      {
         return ((const dmsRecord*)this)->isNormal() ;
      }

      void resetAttr()
      {
         return ((dmsRecord*)this)->resetAttr() ;
      }

      void setData( const dmsRecordData &data )
      {
         return ((dmsRecord*)this)->setData(data) ;
      }

      const CHAR* getData() const
      {
         return ((const dmsRecord*)this)->getData() ;
      }

      UINT32 getDataLength() const
      {
         return ((const dmsRecord*)this)->getDataLength() ;
      }

      BOOLEAN isCompressed() const
      {
         return ((const dmsRecord*)this)->isCompressed() ;
      }

      UINT8 getCompressType () const
      {
         return ((const dmsRecord *)this)->getCompressType() ;
      }

      BYTE getState() const
      {
         return ((const dmsRecord*)this)->getState() ;
      }
   } ;
   typedef _dmsCappedRecord dmsCappedRecord ;


   /*
      _dmsDeletedRecord defined
   */
   class _dmsDeletedRecord : public SDBObject
   {
   public :
      union
      {
         CHAR     _recordHead[4] ;     // 1 byte flag, 3 bytes length-1
         UINT32   _flag_and_size ;
      }                 _head ;
      dmsOffset         _myOffset ;
      dmsRecordID       _next ;
      // FIXME: Enable this once we switch default record to V1
      // DPS_TRANS_ID     _globTransID ;  // the position of GlobTransID is same 
                                          // as V1 Record. So once a record is
                                          // deleted under new release, it's 
                                          // automatically converted to V1 type

      /*
         Get Functions
      */
      CHAR getFlag() const
      {
         return _head._recordHead[ 0 ] ;
      }
      BOOLEAN isDeleted() const
      {
         return ( ( DMS_RECORD_FLAG_DELETED | getFlag() ) 
                  == DMS_RECORD_FLAG_DELETED ) ;
      }
      UINT32 getSize() const
      {
         return ((const dmsRecord*)this)->getSize() ;
      }
      dmsOffset getMyOffset() const
      {
         return _myOffset ;
      }
      dmsRecordID getNextRID() const
      {
         return _next ;
      }

      /*
         Set Functions
      */
      void setSize( UINT32 size )
      {
         return ((dmsRecord*)this)->setSize( size ) ;
      }
      void setMyOffset( dmsOffset myOffset )
      {
         _myOffset = myOffset ;
      }
      void setNextRID( const dmsRecordID &rid )
      {
         _next = rid ;
      }
      void setFlag( CHAR flag )
      {
         _head._recordHead[ 0 ] = flag ;
      }
      void setDeleted()
      {
         setFlag( DMS_RECORD_FLAG_DELETED ) ;
      }
#if 0    // FIXME:enable this later
      void setHasGlobTransID()
      {
         _head._recordHead[ 0 ] |= DMS_RECORD_FLAG_HASGLOBTRANSID ;
      }
      void resetGlobTransID()
      {
         setHasGlobTransID() ;
         _globTransID = DPS_INVALID_TRANS_ID ;
      }
#endif
   } ;
   typedef _dmsDeletedRecord dmsDeletedRecord ;
   #define DMS_DELETEDRECORD_METADATA_SZ  sizeof(dmsDeletedRecord)

   // oid + one field = 12 + 5 = 17, Algned:20
   #define DMS_MIN_DELETEDRECORD_SZ    (DMS_DELETEDRECORD_METADATA_SZ+20)
   #define DMS_MIN_RECORD_SZ           DMS_MIN_DELETEDRECORD_SZ
   // The min size of BSONObj data.
   #define DMS_MIN_RECORD_DATA_SZ      5
}

#endif //DMSRECORD_HPP_

