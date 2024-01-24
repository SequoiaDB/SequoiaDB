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

   Source File Name = dmsTaskStatus.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date      Who      Description
   ====== ========= =======  ==============================================
          2019/8/23 Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_INDEX_STATUS_HPP_
#define DMS_INDEX_STATUS_HPP_

#include "ossTypes.hpp"
#include "ossMemPool.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "msgDef.hpp"
#include "dms.hpp"
#include "pmd.hpp"
#include "../bson/bson.hpp"
#include "ixm.hpp"
using namespace bson ;

namespace engine
{
   // It is consistent with DMS_TASK_TYPE.
   // NEVER change the value, because DMS_TASK_TYPE was written in SYSTASKS.
   enum DMS_TASK_TYPE
   {
      DMS_TASK_SPLIT          = 0,  // split task
      DMS_TASK_SEQUENCE       = 1,  // clear sequence cache on coords task
      DMS_TASK_CREATE_IDX     = 2,  // create index task
      DMS_TASK_DROP_IDX       = 3,  // drop index task
      DMS_TASK_COPY_IDX       = 4,  // copy index task

      DMS_TASK_UNKNOWN        = 255
   } ;

   // It is consistent with CLS_TASK_STATUS.
   // NEVER change the value, because CLS_TASK_STATUS was written in SYSTASKS.
   enum DMS_TASK_STATUS
   {
      DMS_TASK_STATUS_READY    = 0, // when it's created
      DMS_TASK_STATUS_RUN      = 1, // when it starts running
      DMS_TASK_STATUS_PAUSE    = 2, // when it's halt
      DMS_TASK_STATUS_CANCELED = 3, // when it's canceled by user
      DMS_TASK_STATUS_META     = 4, // when its meta changed ( ex:catalog info )
      DMS_TASK_STATUS_CLEANUP  = 5, // when it cleans up some data
      DMS_TASK_STATUS_ROLLBACK = 6, // when it rolls back

      DMS_TASK_STATUS_FINISH   = 9, // when it's finished, whether succ or fail
      DMS_TASK_STATUS_END      = 10 // no task should has this status
   } ;

   enum DMS_OPINFO_TYPE
   {
      OPINFO_SCAN_DATA        = 0,
      OPINFO_SORT_DATA        = 1,
      OPINFO_INSERT_KEY       = 2,
      OPINFO_SCAN_THEN_INSERT = 3, // scan data then insert key

      OPINFO_UNKNOWN          = 255
   } ;

   #define DMS_OPINFO_STR_SCANDATA       "Scanning data, and loading them " \
                                         "into buffer"
   #define DMS_OPINFO_STR_SORTDATA       "Sorting data in the buffer"
   #define DMS_OPINFO_STR_INSERTKEY      "Inserting buffer's keys to index tree"
   #define DMS_OPINFO_STR_SCANTHENINSERT "Scanning each record, and then " \
                                         "inserting index key to index tree"

   #define DMS_INVALID_TASKID     ( 0 )

   #define DMS_TASK_MASK_ALL                       (~0)

   #define DMS_TASK_MASK_GROUPNAME                 0x00000001
   #define DMS_TASK_MASK_MAINTASKID                0x00000002
   #define DMS_TASK_MASK_TASKID                    0x00000004
   #define DMS_TASK_MASK_STATUS                    0x00000008
   #define DMS_TASK_MASK_TASKTYPE                  0x00000010
   #define DMS_TASK_MASK_CLNAME                    0x00000020
   #define DMS_TASK_MASK_IDXNAME                   0x00000040
   #define DMS_TASK_MASK_IDXDEF                    0x00000080
   #define DMS_TASK_MASK_SORTBUFSZ                 0x00000100
   #define DMS_TASK_MASK_RESULTCODE                0x00000200
   #define DMS_TASK_MASK_RESULTINFO                0x00000400
   #define DMS_TASK_MASK_OPINFO                    0x00000800
   #define DMS_TASK_MASK_RETRYCNT                  0x00001000
   #define DMS_TASK_MASK_PROGRESS                  0x00002000
   #define DMS_TASK_MASK_SPEED                     0x00004000
   #define DMS_TASK_MASK_TIMESPENT                 0x00008000
   #define DMS_TASK_MASK_TIMELEFT                  0x00010000
   #define DMS_TASK_MASK_TOTALSZ                   0x00020000
   #define DMS_TASK_MASK_PROCESSSZ                 0x00040000

   // for db.snapshot(SDB_SNAP_TASKS)
   #define DMS_TASK_MASK_SNAPSHOT  ( ~DMS_TASK_MASK_GROUPNAME )

   // for reporting task to catalog
   #define DMS_TASK_MASK_REPORT    ( DMS_TASK_MASK_GROUPNAME   | \
                                     DMS_TASK_MASK_TASKID      | \
                                     DMS_TASK_MASK_STATUS      | \
                                     DMS_TASK_MASK_RESULTCODE  | \
                                     DMS_TASK_MASK_RESULTINFO  | \
                                     DMS_TASK_MASK_OPINFO      | \
                                     DMS_TASK_MASK_RETRYCNT    | \
                                     DMS_TASK_MASK_PROGRESS    | \
                                     DMS_TASK_MASK_SPEED       | \
                                     DMS_TASK_MASK_TIMESPENT   | \
                                     DMS_TASK_MASK_TIMELEFT    | \
                                     DMS_TASK_MASK_TOTALSZ     | \
                                     DMS_TASK_MASK_PROCESSSZ )

   class _dmsTaskStatus : public SDBObject
   {
      public:
         _dmsTaskStatus( UINT64 taskID ) : _taskID( taskID ) {}
         virtual ~_dmsTaskStatus () {}

         virtual UINT64 taskID() const { return _taskID ; }

         virtual DMS_TASK_TYPE taskType() const = 0 ;
         virtual DMS_TASK_STATUS status() const = 0 ;
         virtual void setStatus( DMS_TASK_STATUS status ) = 0 ;
         virtual INT32 resultCode() const = 0 ;
         virtual BSONObj toBSON( UINT32 mask = DMS_TASK_MASK_ALL ) = 0 ;

         virtual const CHAR* collectionName() const = 0 ;
         virtual void collectionRename( const CHAR* newCLName ) = 0 ;

         virtual const ossTimestamp& beginTimestamp() const = 0 ;
         virtual const ossTimestamp& endTimestamp() const = 0 ;

         // The local task has a corresponding task on catalog.
         virtual BOOLEAN hasTaskInCatalog()   { return FALSE ; }
         // The corresponding task on catalog has been finished or removed.
         virtual void setHasTaskInCatalog( BOOLEAN has ) {}

         virtual void updateProgress() {}

         virtual BOOLEAN pauseReport() { return FALSE ; }
         virtual void setPauseReport( BOOLEAN is ) {}

      protected:
         UINT64 _taskID ;
   } ;
   typedef _dmsTaskStatus dmsTaskStatus ;

   class _dmsIdxTaskStatus : public _dmsTaskStatus
   {
      public:
         _dmsIdxTaskStatus( DMS_TASK_TYPE taskType,
                            UINT64 taskID,
                            UINT32 locationID,
                            UINT64 mainTaskID = DMS_INVALID_TASKID ) ;
         _dmsIdxTaskStatus& operator=( const _dmsIdxTaskStatus &rhs ) ;
         virtual ~_dmsIdxTaskStatus () {}

         virtual DMS_TASK_TYPE taskType() const { return _taskType ; }
         virtual DMS_TASK_STATUS status() const { return _taskStatus ; }
         virtual void setStatus( DMS_TASK_STATUS status ) ;
         virtual INT32 resultCode() const { return _resultCode ; }
         virtual BSONObj toBSON( UINT32 mask = DMS_TASK_MASK_ALL ) ;

         virtual const CHAR* collectionName() const ;
         virtual void collectionRename( const CHAR* newCLName ) ;

         virtual const ossTimestamp& beginTimestamp() const ;
         virtual const ossTimestamp& endTimestamp() const ;

         virtual void updateProgress() ;

         // The local task has a corresponding unfinished task on catalog.
         virtual BOOLEAN hasTaskInCatalog() ;
         // The corresponding task on catalog has been finished or removed.
         virtual void setHasTaskInCatalog( BOOLEAN has ) ;

         virtual BOOLEAN pauseReport() { return _pauseReport ; }
         virtual void setPauseReport( BOOLEAN is ) { _pauseReport = is ; }

         void    setStatus2Finish( INT32 resultCode,
                                   const CHAR* resultDetail = NULL,
                                   utilWriteResult* wResultDetail = NULL ) ;

         UINT64 locationID() const { return _locationID ; }
         void   setLocationID( UINT32 id ) { _locationID = id ; }

         UINT64 mainTaskID() const { return _mainTaskID ; }

         void collectionName( CHAR* name, INT32 size ) const ;
         utilCLUniqueID clUniqueID() const { return _clUniqueID ; }

         const CHAR* indexName() const { return _indexName ; }

         INT32   init( const CHAR* collectionName,
                       const BSONObj& index,
                       INT32 sortBufSize = -1,
                       utilCLUniqueID clUniqID = UTIL_UNIQUEID_NULL ) ;
         BOOLEAN isInitialized() const { return _isInitialized ; }

         INT32   sortBufSize() { return _sortBufSize ; }

         void    setTotalRecNum( UINT64 num ) ;

         ossAtomic64* pcsedRecNumPtr() { return &_pcsedRecNum ; }
         void    incPcsedRecNum( UINT64 delta ) ;
         void    resetPcsedRecNum() ;

         void    incRetryCnt() ;

         void    setOpInfo( DMS_OPINFO_TYPE infoType ) ;
         void    resetOpInfo() ;

         const BSONObj& indexDef() const ;
         INT32   setIndexDef( const BSONObj &indexDef ) ;
         BOOLEAN isGlobalIdx() const ;
         INT32   globalIdxCL( const CHAR *&clName, UINT64 &clUniqID ) const ;

      private:
         void _buildResultInfo( INT32 resultCode,
                                const CHAR* resultDetail,
                                utilWriteResult* wResultDetail ) ;

      private:
         DMS_TASK_TYPE    _taskType ;
         UINT32           _locationID ;     // locationID in data
         UINT64           _mainTaskID ;     // main taskID

         volatile DMS_TASK_STATUS _taskStatus ;

         utilCLUniqueID   _clUniqueID ;
         CHAR             _indexName[ IXM_INDEX_NAME_SIZE ] ;
         INT32            _sortBufSize ;

         ossTimestamp     _beginTimestamp ;
         volatile UINT32  _retryCnt ;
         volatile INT32   _resultCode ;
         volatile DMS_OPINFO_TYPE _opInfo ;

         volatile BOOLEAN _isInitialized ;
         volatile BOOLEAN _pauseReport ;
         BOOLEAN          _hasCatalogTask ;

         volatile BOOLEAN _hasSetDef ;      // protect _indexDef
         BSONObj          _indexDef ;
         BOOLEAN          _isGlobalIdx ;

         mutable ossSpinSLatch _nameLatch ; // protect _clFullName
         CHAR             _clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;

         mutable ossSpinSLatch _latch ;     // protect below variable
         BSONObj          _resultInfo ;
         ossTimestamp     _endTimestamp ;
         ossTimestamp     _calculateTimestamp ;
         ossAtomic64      _totalRecNum ;
         ossAtomic64      _pcsedRecNum ;
         UINT64           _pcsRecNumLastTime ;
         UINT32           _progress ;
         UINT64           _speed ;
         FLOAT64          _timeSpent ;
         FLOAT64          _timeLeft ;
   } ;
   typedef _dmsIdxTaskStatus dmsIdxTaskStatus ;

   #define DMS_IDX_ALL                 (~0)
   #define DMS_IDX_NORMAL              0x00000001
   #define DMS_IDX_STANDALONE          0x00000002

   /*
      The first bit ditinguish between real catalog taskid and dummy taskid.
      Task id generated by dms is in range of ( min, max ]
   */
   #define DMS_DUMMY_CATTASKID_MIN      0x7FFFFFFFFFFFFFFF
   #define DMS_DUMMY_CATTASKID_MAX      0xFFFFFFFFFFFFFFFF

   #define DMS_IS_DUMMY_CATTASKID( id ) ( id & 0x8000000000000000 )

   typedef boost::shared_ptr<dmsTaskStatus>    dmsTaskStatusPtr ;
   typedef boost::shared_ptr<dmsIdxTaskStatus> dmsIdxTaskStatusPtr ;

   typedef ossPoolMap<UINT64, dmsTaskStatusPtr> MAP_STATUS ;
   typedef MAP_STATUS::iterator MAP_IDSTATUS_IT ;

   /*
    * dmsTaskStatusMgr
    */
   class _dmsTaskStatusMgr: public SDBObject
   {
      public:
         _dmsTaskStatusMgr() : _dummyTaskHWM( DMS_DUMMY_CATTASKID_MIN )
         {}
         ~_dmsTaskStatusMgr() {}

         INT32 createIdxItem( DMS_TASK_TYPE type,
                              dmsIdxTaskStatusPtr& statusPtr,
                              UINT64 taskID = DMS_INVALID_TASKID,
                              UINT32 locationID = 0,
                              UINT64 mainTaskID = DMS_INVALID_TASKID ) ;

         BOOLEAN findItem( UINT64 taskID,
                           dmsTaskStatusPtr& statusPtr ) ;

         INT32 dumpInfo( ossPoolMap<UINT64, BSONObj>& statusMap ) ;

         INT32 dumpForReport( ossPoolMap<UINT64, BSONObj>& statusMap ) ;

         BOOLEAN hasTaskToReport();

         void cleanOutOfDate( BOOLEAN isPrimary ) ;

         void renameCS( const CHAR* oldCSName,
                        const CHAR* newCSName ) ;

         void renameCL( const CHAR* oldCLName,
                        const CHAR* newCLName ) ;

         void dropCS( const CHAR* csName ) ;

         void dropCL( const CHAR* collection ) ;

      private:
         MAP_STATUS    _mapStatus  ; // map< taskID, status >
         ossSpinSLatch _mapLatch ;

         UINT64        _dummyTaskHWM ; // high water mark of dummy task id
         ossSpinSLatch _hwmLatch ;
   } ;
   typedef _dmsTaskStatusMgr dmsTaskStatusMgr ;
}

#endif /* DMS_INDEX_STATUS_HPP_ */

