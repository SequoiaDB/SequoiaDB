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

   Source File Name = dms.hpp

   Descriptive Name = Data Management Service Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   dms Reccord ID (RID).

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_HPP_
#define DMS_HPP_
#include "core.hpp"
#include "oss.hpp"
#include "utilUniqueID.hpp"

#include <string>
#include <set>

namespace engine
{
#define DMS_COLLECTION_SPACE_NAME_SZ      127
// page length can be 4/8/16/32/64K
// Note that windows memory allocation granulartiy is 64K, so we need to make
// sure each segment must be multiple of 64K

#define DMS_PAGE_SIZE4K        4096    // 4K
#define DMS_PAGE_SIZE8K        8192    // 8K
#define DMS_PAGE_SIZE16K       16384   // 16K
#define DMS_PAGE_SIZE32K       32768   // 32K
#define DMS_PAGE_SIZE64K       65536   // 64K

/// for lobm
#define DMS_PAGE_SIZE64B       64
#define DMS_PAGE_SIZE256B      256

#define DMS_PAGE_SIZE128K      131072  // 128K
#define DMS_PAGE_SIZE256K      262144  // 256K
#define DMS_PAGE_SIZE512K      524288  // 512K

#define DMS_PAGE_SIZE_DFT      DMS_PAGE_SIZE64K
#define DMS_PAGE_SIZE_MAX      DMS_PAGE_SIZE64K
#define DMS_PAGE_SIZE_BASE     DMS_PAGE_SIZE4K

#define DMS_PAGE_SIZE_LOG2_BASE ( 12 ) // 2 ^ 12 = 4KB
#define DMS_PAGE_SIZE_LOG2_DFT  ( 16 ) // 2 ^ 16 = 64KB

#define DMS_DEFAULT_LOB_PAGE_SZ  DMS_PAGE_SIZE256K
#define DMS_DO_NOT_CREATE_LOB    0

#define DMS_LOG_WRITE_MOD_INCREMENT 0
#define DMS_LOG_WRITE_MOD_FULL      1


// the maximum number of pages * size for the storage unit
// this number does NOT count metadata
// max SU size:
// 4K: 512GB
// 8K: 1TB
// 16K: 2TB
// 32K: 4TB
// 64K: 8TB
// Note this number is 2^28
#define DMS_MAX_PG             (128*1024*1024)
#define DMS_MAX_SZ(x)          (((UINT64)DMS_MAX_PG)*(x))

// fixed segment size 128MB
#define DMS_SEGMENT_SZ_BASE    (32*1024*1024)      /// 32MB
#define DMS_SEGMENT_SZ_MAX     (512*1024*1024)     /// 512MB

#define DMS_SEGMENT_SZ         (128*1024*1024)     /// 128MB
#define DMS_SEGMENT_PG(s,x)    ((s)/(x))

#define DMS_SYS_SEGMENT_SZ     (32*1024*1024)      /// 32MB

#define DMS_IS_VALID_SEGMENT(x)  ((x) > 0 && \
                                  (x) <= DMS_SEGMENT_SZ_MAX && \
                                  (x) % DMS_SEGMENT_SZ_BASE == 0)

#define DMS_MAX_EXTENT_SZ(p)   (p->getSegmentSize())
#define DMS_MIN_EXTENT_SZ(x)   (x)
#define DMS_BEST_UP_EXTENT_SZ  (16*1024*1024)

#define DMS_ID_KEY_NAME        "_id"

#define DMS_COLLECTION_NAME_SZ      127
#define DMS_COLLECTION_MAX_INDEX    64
#define DMS_MME_SLOTS               4096
#define DMS_COLLECTION_FULL_NAME_SZ \
   ( DMS_COLLECTION_SPACE_NAME_SZ + DMS_COLLECTION_NAME_SZ + 1 )

#define DMS_INVALID_SUID            -1
#define DMS_INVALID_CLID            ~0
#define DMS_INVALID_OFFSET          -1
#define DMS_INVALID_EXTENT          -1
#define DMS_MAX_SCANNED_EXTENT      OSS_SINT32_MAX
#define DMS_INVALID_MBID            65535
#define DMS_INVALID_PAGESIZE        0
#define DMS_INVALID_LOGICCSID       0xffffffff
#define DMS_INVALID_LOGICCLID       0xffffffff

#define DMS_DATA_SU_EXT_NAME        "data"
#define DMS_INDEX_SU_EXT_NAME       "idx"
#define DMS_LOB_DATA_SU_EXT_NAME    "lobd"
#define DMS_LOB_META_SU_EXT_NAME    "lobm"

#define SDB_DMSTEMP_NAME            "SYSTEMP"
#define DMS_TEMP_NAME_PATTERN       "%s%04d"

#define SDB_DMSRBS_NAME            "SYSRBS"
#define SDB_DMSRBS_FULLNAME        "SYSRBS.SYSRBS"
#define DMS_RBS_NAME_PATTERN       "%s%04d"
#define DMS_MAX_RBS_CL             DMS_MME_SLOTS 
#define DMS_FIRST_RBS_CL           1 
#define DMS_META_RBS_CL            0

#define DMS_INDEX_SORT_BUFFER_MIN_SIZE     32

#define DMS_CAP_EXTENT_SZ           (32 * 1024 * 1024)
#define DMS_CAP_EXTENT_BODY_SZ      ( DMS_CAP_EXTENT_SZ - DMS_EXTENT_METADATA_SZ )

// Unit is MB. This is the upper limit. It should be smaller than the maximum
// size of the storage unit.
#define DMS_CAP_CL_SIZE             ( OSS_SINT64_MAX >> 20 )

#define DMS_MAX_CL_SIZE_ALIGN_SIZE  ( 32 * 1024 * 1024 )

#define DMS_MAX_EXT_NAME_SIZE       DMS_COLLECTION_SPACE_NAME_SZ

// Default size of Rollback Segment collection is 128MB each.
#define DMS_DFT_RBSCL_SIZE          ( 128 * 1024 * 1024 )

   /*
      MB FLAG(_flag) values :
   */
#define DMS_MB_BASE_MASK                        0x000F
   // BASE MASK 0~3 bit
#define DMS_MB_FLAG_FREE                        0x0000
#define DMS_MB_FLAG_USED                        0x0001
#define DMS_MB_FLAG_DROPED                      0x0002

#define DMS_MB_OPR_TYPE_MASK                    0x00F0
   // OPR MASK 4~7 bit
#define DMS_MB_FLAG_OFFLINE_REORG               0x0010
#define DMS_MB_FLAG_ONLINE_REORG                0x0020
#define DMS_MB_FLAG_LOAD                        0x0040

#define DMS_MB_OPR_PHASE_MASK                   0x0F00
   // OPR PHASE 8~11 bit

   // {{ DMS_MB_FLAG_OFFLINE_REORG OPR BEGIN
#define DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY   0x0100
#define DMS_MB_FLAG_OFFLINE_REORG_TRUNCATE      0x0200
#define DMS_MB_FLAG_OFFLINE_REORG_COPY_BACK     0x0400
#define DMS_MB_FLAG_OFFLINE_REORG_REBUILD       0x0800
   // DMS_MB_FLAG_OFFLINE_REORG OPR END }}

   // {{ DMS_MB_FLAG_LOAD OPR BEGIN
#define DMS_MB_FLAG_LOAD_LOAD                   0x0100
#define DMS_MB_FLAG_LOAD_BUILD                  0x0200
   // DMS_MB_FLAG_LOAD OPR END }}

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

   /*
      DMS TOOL FUNCTIONS:
   */
   enum _DMS_ACCESS_TYPE
   {
      DMS_ACCESS_TYPE_NULL  = 0,
      DMS_ACCESS_TYPE_QUERY,
      DMS_ACCESS_TYPE_FETCH,
      DMS_ACCESS_TYPE_INSERT,
      DMS_ACCESS_TYPE_UPDATE,
      DMS_ACCESS_TYPE_DELETE,
      DMS_ACCESS_TYPE_TRUNCATE,
      DMS_ACCESS_TYPE_CRT_INDEX,
      DMS_ACCESS_TYPE_DROP_INDEX,
      DMS_ACCESS_TYPE_CRT_DICT,
      DMS_ACCESS_TYPE_POP
   } ;
   typedef enum _DMS_ACCESS_TYPE DMS_ACCESS_TYPE ;

   typedef SINT32 dmsStorageUnitID ;
   typedef SINT32 dmsExtentID ;
   typedef SINT32 dmsOffset ;

   /*
      _dmsRecordID defined
   */
   class _dmsRecordID : public SDBObject
   {
   public :
      dmsExtentID _extent ;
      dmsOffset   _offset ;

      _dmsRecordID ()
      {
         _extent = DMS_INVALID_EXTENT ;
         _offset = DMS_INVALID_OFFSET ;
      }
      _dmsRecordID ( dmsExtentID extent, dmsOffset offset )
      {
         _extent = extent ;
         _offset = offset ;
      }
      _dmsRecordID& operator=(const _dmsRecordID &rhs)
      {
         _extent=rhs._extent;
         _offset=rhs._offset;
         return *this;
      }
      BOOLEAN isNull () const
      {
         return (DMS_INVALID_EXTENT == _extent) ;
      }
      BOOLEAN isValid () const
      {
         return DMS_INVALID_EXTENT != _extent ;
      }
      BOOLEAN operator!=(const _dmsRecordID &rhs) const
      {
         return !(_extent == rhs._extent &&
                  _offset == rhs._offset) ;
      }
      BOOLEAN operator==(const _dmsRecordID &rhs) const
      {
         return ( _extent == rhs._extent &&
                  _offset == rhs._offset) ;
      }
      BOOLEAN operator<=(const _dmsRecordID &rhs) const
      {
         return compare(rhs)<=0 ;
      }
      BOOLEAN operator<(const _dmsRecordID &rhs) const
      {
         return compare(rhs)<0 ;
      }
      // <0 if current object sit before argment rid
      // =0 means extent/offset are the same
      // >0 means current obj sit after argment rid
      INT32 compare ( const _dmsRecordID &rhs ) const
      {
         if (_extent != rhs._extent)
            return _extent - rhs._extent ;
         return _offset - rhs._offset ;
      }
      void reset()
      {
         _extent = DMS_INVALID_EXTENT ;
         _offset = DMS_INVALID_OFFSET ;
      }
      void resetMax()
      {
         _extent = 0x7FFFFFFF ;
         _offset = 0x7FFFFFFF ;
      }
      void resetMin()
      {
         _extent = 0 ;
         _offset = 0 ;
      }
   } ;
   typedef class _dmsRecordID dmsRecordID ;

   /*
      DMS_FILE_TYPE define
   */
   typedef UINT32 DMS_FILE_TYPE ;

   #define DMS_FILE_EMPTY  ( 0x00000000 )
   #define DMS_FILE_DATA   ( 0x00000001 )
   #define DMS_FILE_IDX    ( 0x00000002 )
   #define DMS_FILE_LOB    ( 0x00000004 )
   #define DMS_FILE_ALL    ( 0xFFFFFFFF )

   enum DMS_STORAGE_TYPE
   {
      DMS_STORAGE_NORMAL = 0,
      DMS_STORAGE_CAPPED,
      DMS_STORAGE_DUMMY
   } ;

   /*
      DMS Other define
   */
#define DMS_MON_OP_COUNT_INC( _pMonAppCB_, op, delta )     \
   {                                                       \
      if ( NULL != _pMonAppCB_ )                           \
      {                                                    \
         _pMonAppCB_->monOperationCountInc( op, delta ) ;  \
      }                                                    \
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

   // helper function, check DMS/IXM object name validity
   BOOLEAN  dmsIsSysCSName ( const CHAR *collectionSpaceName ) ;
   INT32    dmsCheckCSName ( const CHAR *collectionSpaceName,
                             BOOLEAN sys = FALSE ) ;

   BOOLEAN  dmsIsSysCLName ( const CHAR *collectionName ) ;
   INT32    dmsCheckCLName ( const CHAR *collectionName,
                             BOOLEAN sys = FALSE ) ;
   INT32    dmsCheckFullCLName ( const CHAR *collectionName,
                                 BOOLEAN sys = FALSE ) ;
   BOOLEAN  dmsIsSysIndexName ( const CHAR *indexName ) ;
   INT32    dmsCheckIndexName ( const CHAR *indexName,
                                BOOLEAN sys = FALSE ) ;

   std::string dmsGetCSNameFromFullName( const std::string &fullName ) ;

   std::string dmsGetCLShortNameFromFullName( const std::string &fullName ) ;

}

#endif /* DMS_HPP_ */

