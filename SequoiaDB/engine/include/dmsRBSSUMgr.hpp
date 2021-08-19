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

   Source File Name = dmsRBSSUMgr.hpp

   Descriptive Name = DMS Roll Back Segment Storage Unit Management Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS RollBack Segment Storage Unit Management.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/10/2019  CYX Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSRBSSUMGR_HPP__
#define DMSRBSSUMGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossLatch.hpp"
#include "ossUtil.hpp"
#include "dms.hpp"
#include "dmsSysSUMgr.hpp"
#include "dmsRecord.hpp"
#include "dmsStorageDataCommon.hpp"
#include "dpsDef.hpp"

using namespace std ;

namespace engine
{
   // number of slots in RBS hash bucket, a prime number less than 32K
   #define  DMS_RBS_HASH_BKT_SLOTS   ( (UINT32) 32749 )

   // record offset within RBS
   class dmsRBSOffset
   {
   public:
      UINT16  _clID ;           // RBS collection id, it's the same as the
                                // one in rbs cl name (1-4095)
                                // not the collection logical id
      INT64   _logicalID ;      // offset within the collection
   public:
      dmsRBSOffset()
      {
         _clID = DMS_INVALID_CLID ;
         _logicalID = -1 ;
      }
 
      BOOLEAN  operator==(const dmsRBSOffset &rhs) const
      {
         return ((_clID == rhs._clID) && (_logicalID == rhs._logicalID) ) ;
      }

      dmsRBSOffset&  operator=(const dmsRBSOffset &rhs)
      {
         _clID = rhs._clID ;
         _logicalID = rhs._logicalID ;
         return *this ;
      }

      OSS_INLINE BOOLEAN isValid() const
      {
         BOOLEAN rv = TRUE ;
         if ( _clID == DMS_INVALID_CLID || _logicalID == -1 )
         { 
            rv = FALSE ;
         }
         return rv ;
      }
   } ;

   // Rollback segment record key, used for hash and match
   class dmsRBSRecordKey
   {
   public:
      // original record CSID, CLID and rid.
      dmsStorageUnitID  _csID ; 
      UINT16            _clID ;
      dmsRecordID       _rid ; 

   public:
      dmsRBSRecordKey()
      {
         _csID = DMS_INVALID_SUID ;
         _clID = DMS_INVALID_CLID ;
         _rid.reset() ;  
      }

      dmsRBSRecordKey( dmsStorageUnitID  csid,
                       UINT16            clid,
                       dmsRecordID       rid)
      {
         _csID = csid ;
         _clID = clid ;
         _rid  = rid ;  
      }

      BOOLEAN  operator==(const dmsRBSRecordKey&rhs) const
      {
         return ( ( _csID == rhs._csID ) && 
                  ( _clID == rhs._clID ) &&
                  ( _rid == rhs._rid ) ) ;
      }

      BOOLEAN  operator!=(const dmsRBSRecordKey&rhs) const
      {
         return ( ( _csID != rhs._csID ) ||
                  ( _clID != rhs._clID ) ||
                  ( _rid != rhs._rid ) ) ;
      }
      dmsRBSRecordKey&  operator=(const dmsRBSRecordKey &rhs)
      {
         _csID = rhs._csID ;
         _clID = rhs._clID ;
         _rid = rhs._rid ;
         return *this ;
      }
      BOOLEAN  operator<(const dmsRBSRecordKey &rhs) const
      {
         BOOLEAN rv = false ;
         if (  _csID < rhs._csID )
         {
            rv = true ;
         }
         else if ( _csID == rhs._csID )
         {
            if ( _clID < rhs._clID )
            {
               rv = true ; 
            }
            else if ( _clID == rhs._clID )
            {
               rv = (_rid < rhs._rid) ;
            }
         }
         return rv ;
      }
   } ;

   // Define Rollback Segment Record. These records are old version
   // records stored in system capped storage for MVCC feature.
   //   class _dmsRBSRecord : public _dmsCappedRecord
   class _dmsRBSRecord : public SDBObject
   {
   private :
      union
      {
         CHAR     _recordHead[4] ;     // 1 byte flag, 3 bytes length - 1
         UINT32   _flag_and_size ;
      }                _head ;

      // the original record id this version reflected to. it
      // include CSID and RID
      dmsRBSRecordKey  _recordKey ;

      // global transaction ID which generated this version
      DPS_TRANS_ID     _GlobTransID ;
      // offset of previous older version in the rollback segment
      // The mulitple version of the same records(base on RID) are
      // chained together using this field.
      dmsRBSOffset     _preOffset ;

      /*
         Follow _preOffset is the data length(4 bytes):
                If compressed: 4bytes + data
                uncompressed:  data( the first 4bytes is bson size )
      */

   public:
      _dmsRBSRecord()
      : _recordKey(),
        _preOffset()
      {
         _GlobTransID = DPS_INVALID_TRANS_ID ;
         _head._flag_and_size = 0 ;
      }

      // Note that since we added two more fileds here before the data,
      // any methods accessing data must be implemented for this class
      void setGlobTransID( DPS_TRANS_ID gtid )
      {
         this->_GlobTransID = gtid;
      }

      DPS_TRANS_ID getGlobTransID() const
      {
         return this->_GlobTransID ;
      }

      void setPreOffset( dmsRBSOffset &offset )
      {
         this->_preOffset = offset ;
      }

      dmsRBSOffset getPreOffset() const
      {
         return this->_preOffset ;
      }

      void setRecordKey( dmsStorageUnitID  csid,
                         UINT16            clid,
                         dmsRecordID       rid )
      {
         _recordKey._csID = csid ;
         _recordKey._clID = clid ;
         _recordKey._rid = rid ;
      }

      dmsRBSRecordKey getRecordKey( ) const 
      {
         return _recordKey ;
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
      void setAttr( BYTE attr )
      {
         _head._recordHead[ 0 ] |= (BYTE)(attr&0xF0) ;
      }
      void unsetAttr( BYTE attr )
      {
         _head._recordHead[ 0 ] &= (BYTE)(~(attr&0xF0)) ;
      }

      void setCompressed()
      {
         setAttr( DMS_RECORD_FLAG_COMPRESSED ) ;
      }
      void unsetCompressed()
      {
         unsetAttr( DMS_RECORD_FLAG_COMPRESSED ) ;
      }
      BOOLEAN isCompressed() const
      {
         return getAttr() & DMS_RECORD_FLAG_COMPRESSED ;
      }

      //   FIXME: revisit the logic
      void setCompressType ( UINT8 type )
      {
         *(UINT32 *)( (CHAR *)this + DMS_RECORD_RBS_METADATA_SZ ) |=
            ( ( (UINT32)type << 24 ) & 0xFF000000 ) ;
//#if defined (SDB_BIG_ENDIAN)
//         ( (UINT8 *)this + DMS_RECORD_V1_METADATA_SZ )[ 0 ] = type ;
//#else
//         ( (UINT8 *)this + DMS_RECORD_V1_METADATA_SZ )[ 3 ] = type ;
//#endif // SDB_BIG_ENDIAN
      }
      UINT8 getCompressType () const
      {
#if defined (SDB_BIG_ENDIAN)
         return ( (const UINT8 *)this + DMS_RECORD_RBS_METADATA_SZ)[ 0 ] ;
#else
         return ( (const UINT8 *)this + DMS_RECORD_RBS_METADATA_SZ)[ 3 ] ;
#endif // SDB_BIG_ENDIAN
      }

      /*
         Copy the data to disk directly
      */
      void  setData( const dmsRecordData &data )
      {
         SDB_ASSERT( !data.isEmpty(), "record data can't be empty!" ) ;
         if ( data.isCompressed() )
         {
            this->setCompressed() ;
            UINT32 * temp = (UINT32 *)( (CHAR *)this + DMS_RECORD_RBS_METADATA_SZ ) ;
            (*temp) = data.len() ;
            (*temp) |= ( ( (UINT32)data.getCompressType() << 24 ) &
                           0xFF000000 ) ;
            ossMemcpy( (CHAR*)this+DMS_RECORD_RBS_METADATA_SZ+sizeof(UINT32),
                       data.data(), data.len() ) ;
         }
         else
         {
            this->unsetCompressed() ;
            ossMemcpy( (CHAR*)this+DMS_RECORD_RBS_METADATA_SZ,
                       data.data(), data.len() ) ;
         }
      }

      /*
         Get disk data only, if compressed, not uncompressed
      */
      const CHAR* getData() const
      {
         return isCompressed() ?
            ((const CHAR*)this+sizeof(UINT32)+DMS_RECORD_RBS_METADATA_SZ) :
            ((const CHAR*)this+DMS_RECORD_RBS_METADATA_SZ) ;
      }

      UINT32 getDataLength() const
      {
         return ( *(const UINT32 *)( (const CHAR*)this + 
                                  DMS_RECORD_RBS_METADATA_SZ) ) & 0x00FFFFFF ;
      }

      UINT32 size() const
      {
         // always 4 bytes algined
         UINT32 s = DMS_RECORD_RBS_METADATA_SZ + getDataLength() ;
         s = OSS_MIN( DMS_RECORD_MAX_SZ, ossAlignX( s, 4 ) ) ;
         return s ;
      }
   } ;
   typedef _dmsRBSRecord dmsRBSRecord ;

   class _dmsRBSHashValue
   {
   private:
      dmsRBSOffset   _offset ;  // offset on disk
      ossSpinXLatch  _latch ;   // latch to protect the bucket

   public: 
      _dmsRBSHashValue()
      : _offset() ,
        _latch()
      {
      }

      void   lock()
      {
         _latch.get() ;
      }
      void   release()
      {
         _latch.release() ;
      }
      void   setOffset( dmsRBSOffset & o ) 
      {
         _offset = o ;
      }

      dmsRBSOffset & getOffset ()
      {
         return _offset ;
      }
   } ;

   class _dmsRBSSUMgr : public _dmsSysSUMgr
   {
   private :
      // TODO: need latch to protect these two values. The are basically
      // in memory version of the meta record stored in SYSRBS0000. it's for
      // quick look up of current value so we can directly use the collection.

      // The collection currently in use and the previously freed collecion
      // They are protected by metaCL's mbLatch
      UINT16  _currentCollection ;
      UINT16  _lastFreeCollection ;

      // The max size of each collection
      UINT32  _maxCollectionSize ;

      // The hash bucket to point to the head of the record. 
      // we may have different implementation of how to store and access
      // the old versions. Eventually, we may want to cache the newest 
      // old "version" in memory, which could be hanging off the record
      // lock. but in first round, we will simply store everything on disk.
      // Full size is 32k* (40+12)B = 1.6MB
      _dmsRBSHashValue    _rbsRecordBkt[ DMS_RBS_HASH_BKT_SLOTS ] ;
 

   public :
      _dmsRBSSUMgr ( _SDB_DMSCB *dmsCB ) ;

      // this function verify whether RBS collection space exist. If it
      // is not exist then create one. And then reset all temp collections
      SINT32 init() { return SDB_OK ; }
      void   fini() {}

      SINT32 release ( _dmsMBContext *&context ) ;

      SINT32 reserve ( _dmsMBContext **ppContext, UINT64 eduID ) ;

      SINT32 appendRecord ( dmsStorageUnitID  csid,
                            UINT16            clid,
                            dmsRecordID       rid,
                            DPS_TRANS_ID      gtid,
                            const dmsRecordData  &record)  
      { return SDB_OK ; }

      SINT32 appendRecord ( dmsStorageUnitID  csid,
                            UINT16            clid,
                            dmsRecordID       rid,
                            DPS_TRANS_ID      gtid,
                            const BSONObj     &obj )
      { return SDB_OK ; }

      SINT32 getRecord ( dmsStorageUnitID  csid,
                         UINT16            clid,
                         dmsRecordID       rid,
                         DPS_TRANS_ID      gtid,
                         BOOLEAN          &found,
                         dmsRecordData    &record )  
      { return SDB_OK ; }
      void gcRBS ( ) {}

   private:

      SINT32 _getMeta ( UINT16 &curCL, 
                        UINT16 &lastFreeCL, 
                        dmsMBContext *context  ) 
      { return SDB_OK ; }

      // release a collection
      SINT32 _release ( ) 
      { return SDB_OK ; }

      // insert the meta record during create
      SINT32 _insertMeta ( UINT16 curCL, 
                           UINT16 lastFreeCL,
                           dmsMBContext *context,
                           SDB_DPSCB * dpsCB ) 
      { return SDB_OK ; }

      // Update the meta record, usually after swtiching RBSCLs
      SINT32 _updateMeta () 
      { return SDB_OK ; }
      SINT32 _updateMeta ( UINT16 curCL, 
                           UINT16 lastFreeCL,
                           dmsMBContext *context,
                           SDB_DPSCB  *dpsCB ) 
      { return SDB_OK ; }

      // based on csId, clID and rid to hash to a bucket
      OSS_INLINE UINT32 _hash ( dmsStorageUnitID  _csID ,
                                UINT16            _clID ,
                                dmsRecordID       _rid ) ;

      SINT32 _gcRBS ( UINT16 &position ) 
      { return SDB_OK ; }

      SINT32 _allocRBSRecordSpace( UINT32        size,
                                   dmsRBSOffset &newOffset,
                                   pmdEDUCB     *eduCB,
                                   dmsMBContext *metaContext,
                                   dmsMBContext *&clContext ) 
      { return SDB_OK ; }

      SINT32 _writeRecToLocation( dmsRBSOffset         location,
                                  dmsStorageUnitID     csid,
                                  UINT16               clid,
                                  dmsRecordID          rid,
                                  DPS_TRANS_ID         gtid,
                                  const dmsRecordData &recordData,
                                  UINT32               recSize,
                                  pmdEDUCB            *eduCB,
                                  dmsMBContext        *context ) 
      { return SDB_OK ; }

   } ;
   typedef class _dmsRBSSUMgr dmsRBSSUMgr ;

   OSS_INLINE UINT32 _dmsRBSSUMgr::_hash ( dmsStorageUnitID  _csID ,
                                           UINT16            _clID ,
                                           dmsRecordID       _rid ) 
   {
      UINT64 b = 0 ;
      b |= (UINT64)(_csID & 0xFFF) << 52 ;
      b |= (UINT64)(_rid._extent & 0xFFFFFF) << 28 ;
      b |= (_rid._offset & 0xFFFFFFF) ;

      // ossHash use DJB Hash ( Daniel J. Bernstein ) algorithm :
      //   h(i) = h(i-1) * 33 + str[i]
      // bitwise multiplication x << 5 + x it equivalent to x * 33,
      // where the magic 5 comes. However, there is no adequate
      // explaination on why 33 is choosed as multiplier
      return ( ossHash( (CHAR*)&( b ), (sizeof( b )), 5 ) ) % 
               DMS_RBS_HASH_BKT_SLOTS ;
   }


}
#endif //DMSRBSSUMGR_HPP__

