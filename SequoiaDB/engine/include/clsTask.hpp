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

   Source File Name = clsTask.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          17/03/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_TASK_HPP_
#define CLS_TASK_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "clsBase.hpp"
#include "ossLatch.hpp"
#include "clsCatalogAgent.hpp"
#include <string>
#include <map>
#include "../bson/bson.h"
#include "dmsTaskStatus.hpp"

using namespace bson ;

namespace engine
{
   // NEVER change the value, because it was written in SYSTASKS.
   enum CLS_TASK_TYPE
   {
      CLS_TASK_SPLIT           = DMS_TASK_SPLIT,
      CLS_TASK_SEQUENCE        = DMS_TASK_SEQUENCE,
      CLS_TASK_CREATE_IDX      = DMS_TASK_CREATE_IDX,
      CLS_TASK_DROP_IDX        = DMS_TASK_DROP_IDX,
      CLS_TASK_COPY_IDX        = DMS_TASK_COPY_IDX,

      CLS_TASK_UNKNOWN         = DMS_TASK_UNKNOWN
   } ;

   // NEVER change the value, because it was written in SYSTASKS.
   enum CLS_TASK_STATUS
   {
      CLS_TASK_STATUS_READY    = DMS_TASK_STATUS_READY,
      CLS_TASK_STATUS_RUN      = DMS_TASK_STATUS_RUN,
      CLS_TASK_STATUS_PAUSE    = DMS_TASK_STATUS_PAUSE,
      CLS_TASK_STATUS_CANCELED = DMS_TASK_STATUS_CANCELED,
      CLS_TASK_STATUS_META     = DMS_TASK_STATUS_META,
      CLS_TASK_STATUS_CLEANUP  = DMS_TASK_STATUS_CLEANUP,
      CLS_TASK_STATUS_ROLLBACK = DMS_TASK_STATUS_ROLLBACK,

      CLS_TASK_STATUS_FINISH   = DMS_TASK_STATUS_FINISH,
      CLS_TASK_STATUS_END      = DMS_TASK_STATUS_END
   } ;

   #define CLS_INVALID_TASKID                DMS_INVALID_TASKID
   #define CLS_INVALID_LOCATIONID            0

   #define CLS_MASK_ALL                      (~0)

   // split mask
   #define CLS_SPLIT_MASK_ID                 0x00000001
   #define CLS_SPLIT_MASK_TYPE               0x00000002
   #define CLS_SPLIT_MASK_STATUS             0x00000004
   #define CLS_SPLIT_MASK_CLNAME             0x00000008
   #define CLS_SPLIT_MASK_SOURCEID           0x00000010
   #define CLS_SPLIT_MASK_SOURCENAME         0x00000020
   #define CLS_SPLIT_MASK_DSTID              0x00000040
   #define CLS_SPLIT_MASK_DSTNAME            0x00000080
   #define CLS_SPLIT_MASK_BKEY               0x00000100
   #define CLS_SPLIT_MASK_EKEY               0x00000200
   #define CLS_SPLIT_MASK_SHARDINGKEY        0x00000400
   #define CLS_SPLIT_MASK_SHARDINGTYPE       0x00000800
   #define CLS_SPLIT_MASK_PERCENT            0x00001000
   //#define CLS_SPLIT_MASK_LOCKEND            0x00002000
   #define CLS_SPLIT_MASK_UNIQUEID           0x00004000
   #define CLS_SPLIT_MASK_RESULTCODE         0x00008000
   #define CLS_SPLIT_MASK_ENDTIMESTAMP       0x00010000
   //#define CLS_SPLIT_TASK_LOCK_END           "LockEnd"

   /// index mask
   #define CLS_IDX_MASK_STATUS               0x00000001
   // ResultCode / ResultCodeDesc / ResultInfo
   #define CLS_IDX_MASK_RESULT               0x00000002
   // Progress / Speed / TimeSpent / TimeLeft
   #define CLS_IDX_MASK_PROGRESS             0x00000004
   #define CLS_IDX_MASK_GROUPS               0x00000008
   // TotalGroups / SucceededGroups / FailedGroups
   #define CLS_IDX_MASK_GROUPCOUNT           0x00000010
   #define CLS_IDX_MASK_SUBTASKS             0x00000020
   // TotalTasks / SucceededTasks / FailedTasks
   #define CLS_IDX_MASK_TASKCOUNT            0x00000040
   #define CLS_IDX_MASK_BEGINTIME            0x00000080
   #define CLS_IDX_MASK_ENDTIME              0x00000100
   #define CLS_IDX_MASK_OPINFO               0x00000200
   #define CLS_IDX_MASK_RETRYCNT             0x00000400
   #define CLS_IDX_MASK_TOTALREC             0x00000800
   #define CLS_IDX_MASK_PCSEDREC             0x00001000
   // pull_by/push one group / subtask
   #define CLS_IDX_MASK_PULL_GROUP           0x00002000
   #define CLS_IDX_MASK_PULL_SUBTASK         0x00004000
   #define CLS_IDX_MASK_PUSH_GROUP           0x00008000

   const CHAR* clsTaskTypeStr( CLS_TASK_TYPE taskType ) ;
   const CHAR* clsTaskStatusStr( CLS_TASK_STATUS taskStatus ) ;

   class _clsTask : public SDBObject
   {
      public:
         _clsTask ( UINT64 taskID ) : _taskID ( taskID )
         {
            _taskType   = CLS_TASK_UNKNOWN ;
            _status     = CLS_TASK_STATUS_READY ;
            _resultCode = SDB_OK ;
            _mainTaskID = CLS_INVALID_TASKID ;
            _isMainTask = FALSE ;
         }
         virtual ~_clsTask () {}

         UINT64          taskID () const    { return _taskID ; }
         CLS_TASK_STATUS status () const    { return _status ; }
         CLS_TASK_TYPE   taskType () const  { return _taskType ; }
         INT32           resultCode() const { return _resultCode ; }

         BOOLEAN         hasMainTask() const ;
         UINT64          mainTaskID() const { return _mainTaskID ; }
         BOOLEAN         isMainTask() const { return _isMainTask ; }

         virtual INT32       getSubTasks( ossPoolVector<UINT64>& list ) ;
         virtual void        setStatus( CLS_TASK_STATUS status ) ;

         virtual const CHAR* taskName () const = 0 ;
         virtual const CHAR* collectionName() const = 0 ;
         virtual const CHAR* collectionSpaceName() const = 0 ;
         virtual BOOLEAN     muteXOn ( const _clsTask *pOther ) = 0 ;

         virtual const CHAR* commandName() const ;

         virtual INT32       init( const CHAR* objdata ) = 0 ;
         virtual BSONObj     toBson ( UINT32 mask = CLS_MASK_ALL ) = 0 ;

         virtual INT32       buildStartTask( const BSONObj& obj,
                                             BSONObj& updator,
                                             BSONObj& matcher ) ;
         virtual INT32       buildStartTaskBy( const _clsTask* pSubTask,
                                               const BSONObj& obj,
                                               BSONObj& updator,
                                               BSONObj& matcher ) ;

         virtual INT32       buildCancelTask( const BSONObj& obj,
                                              BSONObj& updator,
                                              BSONObj& matcher ) ;
         virtual INT32       buildCancelTaskBy( const _clsTask* pSubTask,
                                                BSONObj& updator,
                                                BSONObj& matcher ) ;

         virtual INT32       buildReportTask( const BSONObj& obj,
                                              BSONObj& updator,
                                              BSONObj& matcher ) ;
         virtual INT32       buildReportTaskBy( const _clsTask* pSubTask,
                                                const ossPoolVector<BSONObj>& subTaskInfoList,
                                                BSONObj& updator,
                                                BSONObj& matcher ) ;

         virtual INT32       buildQuerySubTasks( const BSONObj& obj,
                                                 BSONObj& matcher,
                                                 BSONObj& selector ) ;
         virtual INT32       buildRemoveTaskBy( UINT64 taskID,
                                                const ossPoolVector<BSONObj>& otherSubTasks,
                                                BSONObj& updator,
                                                BSONObj& matcher ) ;

         virtual INT32       toErrInfo( BSONObjBuilder& builder ) ;

         virtual CLS_TASK_STATUS getTaskStatusByGroup( const CHAR* groupName )
         {
            return _status ;
         }

      protected:
         UINT64                  _taskID ;
         CLS_TASK_TYPE           _taskType ;
         CLS_TASK_STATUS         _status ;
         INT32                   _resultCode ;
         UINT64                  _mainTaskID ; // if this task is a sub-task
         BOOLEAN                 _isMainTask ; // if this task is a main-task
   };
   typedef _clsTask clsTask ;

   INT32 clsNewTask( const BSONObj &taskObj,
                     clsTask*& pTask ) ;
   INT32 clsNewTask( CLS_TASK_TYPE taskType,
                     const BSONObj &taskObj,
                     clsTask*& pTask ) ;
   void  clsReleaseTask( clsTask*& pTask ) ;

   class _clsDummyTask : public _clsTask
   {
      public:
         _clsDummyTask ( UINT64 taskID ) ;
         ~_clsDummyTask () ;
         virtual const CHAR*     taskName () const ;
         virtual const CHAR*     collectionName() const ;
         virtual const CHAR*     collectionSpaceName() const ;
         virtual BOOLEAN         muteXOn ( const _clsTask *pOther ) ;
         virtual INT32           init( const CHAR* objdata ) ;
         virtual BSONObj         toBson ( UINT32 mask = CLS_MASK_ALL ) ;
   };

   class _clsSequenceTask : public _clsTask
   {
      public:
         _clsSequenceTask ( UINT64 taskID ) ;
         ~_clsSequenceTask () ;
         virtual const CHAR*     taskName () const ;
         virtual const CHAR*     collectionName() const ;
         virtual const CHAR*     collectionSpaceName() const ;
         virtual BOOLEAN         muteXOn ( const _clsTask *pOther ) ;
         virtual INT32           init( const CHAR* objdata ) ;
         virtual BSONObj         toBson ( UINT32 mask = CLS_MASK_ALL ) ;
   };

   class _clsTaskMgr : public SDBObject
   {
      private:
         // < task point, refrence count >
         typedef std::pair<_clsTask*, INT32> PAIR_TASK_CNT ;
      public:
         _clsTaskMgr ( UINT32 maxLocationID ) ;
         ~_clsTaskMgr () ;

         UINT32      getLocationID () ;

      public:
         UINT32      taskCount () ;
         UINT32      taskCount( CLS_TASK_TYPE type ) ;

         UINT32      taskCountByCL( const CHAR *pCLName ) ;
         UINT32      taskCountByCS( const CHAR *pCSName ) ;
         INT32       waitTaskEvent( INT64 millisec = OSS_ONE_SEC ) ;

         UINT32      idxTaskCount() ;

         INT32       addTask ( _clsTask *pTask,
                               UINT32 locationID,
                               BOOLEAN *pAlreadyExist = NULL ) ;
         INT32       removeTask ( UINT32 locationID,
                                  BOOLEAN *pDeleted = NULL ) ;
         _clsTask*   findTask ( UINT32 locationID ) ;
         void        stopTask ( UINT32 locationID ) ;

      public:
         void        regCollection( const string &clName ) ;
         void        unregCollection( const string &clName ) ;
         void        lockReg( OSS_LATCH_MODE mode = SHARED ) ;
         void        releaseReg( OSS_LATCH_MODE mode = SHARED ) ;
         UINT32      getRegCount( const string &clName,
                                  BOOLEAN noLatch = FALSE ) ;

         ossPoolString dumpTasks( CLS_TASK_TYPE type = CLS_TASK_UNKNOWN ) ;

      private:
         std::map<UINT32, PAIR_TASK_CNT>     _taskMap ; // key is location ID
         ossSpinSLatch                       _taskLatch ;
         ossAutoEvent                        _taskEvent ;

         std::map<string, UINT32>            _mapRegister ;
         ossSpinSLatch                       _regLatch ;

         UINT32                              _locationID ;
         UINT32                              _maxID ;

   };
   typedef _clsTaskMgr clsTaskMgr ;

   class _clsSplitTask : public _clsTask
   {
      public:
         _clsSplitTask ( UINT64 taskID ) ;
         virtual ~_clsSplitTask () ;

         virtual INT32 init ( const CHAR *objdata ) ;
         virtual BSONObj toBson ( UINT32 mask = CLS_MASK_ALL ) ;

         virtual INT32 buildStartTask( const BSONObj& obj,
                                       BSONObj& updator,
                                       BSONObj& matcher ) ;

         virtual INT32 buildCancelTask( const BSONObj& obj,
                                        BSONObj& updator,
                                        BSONObj& matcher ) ;

         INT32   init ( const CHAR *clFullName, INT32 sourceID,
                        const CHAR *sourceName, INT32 dstID,
                        const CHAR *dstName, const BSONObj &bKey,
                        const BSONObj &eKey, FLOAT64 percent,
                        clsCatalogSet &cataSet ) ;

         INT32   calcHashPartition ( clsCatalogSet &cataSet, INT32 groupID,
                                     FLOAT64 percent, BSONObj &bKey,
                                     BSONObj &eKey ) ;

         BOOLEAN isHashSharding() const ;

      public:
         virtual const CHAR*     taskName () const ;
         virtual const CHAR*     collectionName() const ;
         virtual const CHAR*     collectionSpaceName() const ;

         virtual BOOLEAN         muteXOn ( const _clsTask *pOther ) ;

      public:
         utilCLUniqueID          clUniqueID() const ;
         const CHAR*             shardingType () const ;
         const CHAR*             sourceName () const ;
         const CHAR*             dstName () const ;
         UINT32                  sourceID () const ;
         UINT32                  dstID () const ;
         BSONObj                 splitKeyObj () const ;
         BSONObj                 splitEndKeyObj () const ;
         BSONObj                 shardingKey () const ;

      protected:
         BSONObj                 _getOrdering () const ;
         void                    _makeName() ;

      protected:
         std::string             _clFullName ;
         std::string             _csName ;
         std::string             _sourceName ;
         std::string             _dstName ;
         std::string             _shardingType ;
         UINT32                  _sourceID ;
         UINT32                  _dstID ;
         utilCLUniqueID          _clUniqueID ;
         BSONObj                 _splitKeyObj ;
         BSONObj                 _splitEndKeyObj ;
         BSONObj                 _shardingKey ;
         FLOAT64                 _percent ;
         ossTimestamp            _endTS ;
         //BOOLEAN                 _lockEnd ;

      private:
         std::string             _taskName ;

   };
   typedef _clsSplitTask clsSplitTask ;

   struct _clsSubTaskUnit
   {
      _clsSubTaskUnit( UINT64 taskid = CLS_INVALID_TASKID,
                       CLS_TASK_TYPE type = CLS_TASK_UNKNOWN )
      {
         taskID     = taskid ;
         taskType   = type ;
         status     = CLS_TASK_STATUS_READY ;
         resultCode = 0 ;
      }

      void clear()
      {
         taskID     = CLS_INVALID_TASKID ;
         taskType   = CLS_TASK_UNKNOWN ;
         status     = CLS_TASK_STATUS_READY ;
         resultCode = 0 ;
      }

      INT32 init( const CHAR *objdata ) ;
      BSONObj toBson() ;

      UINT64          taskID ;
      CLS_TASK_TYPE   taskType ;
      CLS_TASK_STATUS status ;
      INT32           resultCode ;
   } ;
   typedef _clsSubTaskUnit clsSubTaskUnit ;

   struct _clsIdxTaskGroupUnit
   {
      _clsIdxTaskGroupUnit()
      {
         clear() ;
      }

      void clear()
      {
         groupName.clear() ;
         taskID      = CLS_INVALID_TASKID ;
         status      = CLS_TASK_STATUS_READY ;
         resultCode  = SDB_OK ;
         resultInfo  = BSONObj() ;
         opInfo.clear() ;
         retryCnt    = 0 ;
         progress    = 0 ;
         speed       = 0 ;
         timeSpent   = 0.0 ;
         timeLeft    = 0.0 ;
         totalRecNum = 0 ;
         pcsedRecNum = 0 ;
      }

      ossPoolString toString() const
      {
         CHAR tmp[ 128 ] = { 0 } ;
         ossSnprintf( tmp, sizeof(tmp)-1,
                      "TaskID: %llu, GroupName: %s, "
                      "Status: %d, ResultCode:  %d",
                      taskID, groupName.c_str(), status, resultCode) ;
         return tmp ;
      }

      INT32 init( const CHAR *objdata ) ;
      BSONObj toBson() ;

      ossPoolString           groupName ;

      UINT64                  taskID ;
      CLS_TASK_STATUS         status ;

      // result
      INT32                   resultCode ;
      BSONObj                 resultInfo ;
      ossPoolString           opInfo ;
      UINT32                  retryCnt ;

      // progress
      UINT32                  progress ;
      UINT64                  speed ;
      FLOAT64                 timeSpent ;
      FLOAT64                 timeLeft ;

      // page
      UINT64                  totalRecNum ;
      UINT64                  pcsedRecNum ;
   };
   typedef _clsIdxTaskGroupUnit clsIdxTaskGroupUnit ;

   class _clsIdxTask : public _clsTask
   {
      typedef ossPoolMap<ossPoolString, _clsIdxTaskGroupUnit> MAP_GROUP_INFO ;
      typedef MAP_GROUP_INFO::iterator MAP_GROUP_INFO_IT ;
      typedef ossPoolMap<UINT64, clsSubTaskUnit> MAP_SUBTASK ;
      typedef MAP_SUBTASK::iterator MAP_SUBTASK_IT ;

      public:
         _clsIdxTask( UINT64 taskID ) ;
         virtual ~_clsIdxTask () {}

         virtual void        setStatus( CLS_TASK_STATUS status ) ;
         virtual const CHAR* taskName() const ;
         virtual const CHAR* collectionName() const ;
         virtual const CHAR* collectionSpaceName() const ;
         virtual const CHAR* indexName() const ;
         virtual BOOLEAN     muteXOn ( const _clsTask *pOther ) ;

         virtual INT32       init( const CHAR *objdata ) ;
         virtual BSONObj     toBson( UINT32 mask = CLS_MASK_ALL ) ;
         virtual INT32       checkConflictWithExistTask( const _clsTask *pExistTask ) = 0 ;

         virtual INT32       getSubTasks( ossPoolVector<UINT64>& list ) ;

         virtual INT32       buildRemoveTaskBy( UINT64 taskID,
                                                const ossPoolVector<BSONObj>& otherSubTasks,
                                                BSONObj& updator,
                                                BSONObj& matcher ) ;

         virtual INT32       buildStartTask( const BSONObj& obj,
                                             BSONObj& updator,
                                             BSONObj& matcher ) ;
         virtual INT32       buildStartTaskBy( const _clsTask* pSubTask,
                                               const BSONObj& obj,
                                               BSONObj& updator,
                                               BSONObj& matcher ) ;

         virtual INT32       buildCancelTask( const BSONObj& obj,
                                              BSONObj& updator,
                                              BSONObj& matcher ) ;
         virtual INT32       buildCancelTaskBy( const _clsTask* pSubTask,
                                                BSONObj& updator,
                                                BSONObj& matcher ) ;

         virtual INT32       buildReportTask( const BSONObj& obj,
                                              BSONObj& updator,
                                              BSONObj& matcher ) ;
         virtual INT32       buildReportTaskBy( const _clsTask* pSubTask,
                                                const ossPoolVector<BSONObj>& subTaskInfoList,
                                                BSONObj& updator,
                                                BSONObj& matcher ) ;

         virtual INT32       buildQuerySubTasks( const BSONObj& obj,
                                                 BSONObj& matcher,
                                                 BSONObj& selector ) ;

         virtual INT32       toErrInfo( BSONObjBuilder& builder ) ;

         utilCLUniqueID  clUniqueID() const ;

         const BSONObj&  resultInfo() const ;

         void            setBeginTimestamp() ;
         void            setEndTimestamp() ;

         void            setRun() ;
         void            setFinish( INT32 resultCode = SDB_OK,
                                    const BSONObj& resultInfo = BSONObj() ) ;

         INT32           countGroup() const ;
         INT32           countSubTask() const ;

         INT32           buildMigrateGroup( const CHAR* srcGroup,
                                            const CHAR* dstGroup,
                                            BSONObj& updator,
                                            BSONObj& matcher ) ;

         INT32           buildAddGroup( const CHAR* groupName,
                                        BSONObj& updator,
                                        BSONObj& matcher ) ;
         INT32           buildAddGroupBy( const _clsTask* pSubTask,
                                          const CHAR* groupName,
                                          BSONObj& updator,
                                          BSONObj& matcher ) ;

         INT32           buildRemoveGroup( const CHAR* groupName,
                                           BSONObj& updator,
                                           BSONObj& matcher ) ;
         INT32           buildRemoveGroupBy( const _clsTask* pSubTask,
                                             const ossPoolVector<BSONObj>& subTaskInfoList,
                                             const CHAR* groupName,
                                             BSONObj& updator,
                                             BSONObj& matcher ) ;

         virtual CLS_TASK_STATUS getTaskStatusByGroup( const CHAR* groupName ) ;

      protected:
         virtual INT32 _init( const CHAR *objdata ) = 0 ;
         virtual void  _toBson( BSONObjBuilder &builder ) = 0 ;

         void  _makeName() ;
         void  _calculate() ;

         void  _updateSubTask( clsSubTaskUnit& curSubTask,
                               const clsSubTaskUnit& newSubTask ) ;
         void  _updateGroup( clsIdxTaskGroupUnit& curGroupInfo,
                             const clsIdxTaskGroupUnit& newGroupInfo ) ;
         INT32 _buildNewGroupInfo( const ossPoolVector<BSONObj> &subTaskInfoList,
                                   clsIdxTaskGroupUnit &newGroupInfo ) ;
         void  _updateOtherByGroupInfo() ;
         void  _updateOtherBySubTaskInfo() ;

         void  _clearChangedMask() ;
         void  _toChangeOtherObj( BSONObjBuilder& matcherB,
                                  BSONObjBuilder& setB ) ;
         void  _toChangeSubtaskObj( BSONObjBuilder& matcherB,
                                    BSONObjBuilder& setB,
                                    const clsSubTaskUnit& subTask ) ;
         void  _toChangeGroupObj( BSONObjBuilder& matcherB,
                                  BSONObjBuilder& setB,
                                  const clsIdxTaskGroupUnit& group ) ;
         void  _toChangeGroupObj( BSONObjBuilder& matcherB,
                                  BSONObjBuilder& setB ) ;
         INT32 _toChangedObj( const clsIdxTaskGroupUnit* group,
                              const clsSubTaskUnit* subTask,
                              BSONObj& matcher,
                              BSONObj& updator ) ;
         INT32 _toChangedObj( BSONObj& matcher,
                              BSONObj& updator ) ;

         INT32 _extractAndSetErrInfo( const BSONObj &errInfoArr,
                                      BSONObjBuilder &matcherBuilder,
                                      BSONObjBuilder &setBuilder ) ;

         INT32 _buildStartTask( const clsTask* pSubTask,
                                const BSONObj& obj,
                                BSONObj& updator,
                                BSONObj& matcher ) ;

         BOOLEAN _isSucceedGroup( const clsIdxTaskGroupUnit& groupInfo ) ;
         BOOLEAN _isSucceedTask( const clsSubTaskUnit& subTaskInfo ) ;
         BOOLEAN _isSucceed( CLS_TASK_TYPE taskType,
                             CLS_TASK_STATUS status,
                             INT32 resultCode ) ;

         void _incSucceededGroups() ;
         void _decSucceededGroups() ;
         void _incFailedGroups() ;
         void _decFailedGroups() ;
         void _incTotalGroups() ;
         void _decTotalGroups() ;

         void _incSucceededTasks() ;
         void _decSucceededTasks() ;
         void _incFailedTasks() ;
         void _decFailedTasks() ;
         void _incTotalTasks() ;
         void _decTotalTasks() ;

      protected:

         // name
         ossPoolString         _taskName ;

         CHAR                  _clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
         utilCLUniqueID        _clUniqueID ;
         CHAR                  _csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;
         CHAR                  _indexName[ IXM_INDEX_NAME_SIZE + 1 ] ;

         // result
         BSONObj               _resultInfo ;

         // progress
         UINT32                _progress ;
         UINT64                _speed ;
         FLOAT64               _timeSpent ;
         FLOAT64               _timeLeft ;

         // group details
         MAP_GROUP_INFO        _mapGroupInfo ;
         UINT32                _totalGroups ;
         UINT32                _succeededGroups ;
         UINT32                _failedGroups ;

         // if this task is a main-task
         MAP_SUBTASK           _mapSubTask ;
         UINT32                _totalTasks ;
         UINT32                _succeededTasks ;
         UINT32                _failedTasks ;

         // timestamp
         ossTimestamp          _createTS ;
         ossTimestamp          _beginTS ;
         ossTimestamp          _endTS ;

         // changed info
         INT32                 _changedMask ;
         INT32                 _changedGroupMask ;
         INT32                 _changedSubtaskMask ;
         UINT32                _i ; // eg: number 1 in "SubTasks.$1.TaskID"
         UINT64                _pullSubTaskID ;
         ossPoolString         _pullGroupName ;
         ossPoolString         _pushGroupName ;
   };
   typedef _clsIdxTask clsIdxTask ;

   class _clsCreateIdxTask : public _clsIdxTask
   {
      public:
         _clsCreateIdxTask( UINT64 taskID ) : _clsIdxTask( taskID )
         {
            _taskType = CLS_TASK_CREATE_IDX ;
            _sortBufferSize = SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE ;
         }
         virtual ~_clsCreateIdxTask() {}

         INT32 initTask( const CHAR *clFullName,
                         utilCLUniqueID clUniqID,
                         const BSONObj &index,
                         UINT64 idxUniqID,
                         const vector<string> &groupList,
                         INT32 sortBufSize,
                         UINT64 mainTaskID = CLS_INVALID_TASKID ) ;

         INT32 initMainTask( const CHAR *clFullName,
                             utilCLUniqueID clUniqID,
                             const BSONObj &index,
                             UINT64 idxUniqID,
                             const ossPoolSet<ossPoolString> &groupList,
                             const ossPoolVector<UINT64> &subTaskList ) ;

         virtual const CHAR* commandName() const ;

         virtual INT32 checkConflictWithExistTask( const _clsTask *pExistTask ) ;

         INT32           sortBufSize() const ;
         const BSONObj&  indexDef() const ;
         INT32           globalIdxCL( const CHAR *&clName,
                                      UINT64 &clUniqID ) const ;
         INT32           addGlobalOpt2Def( const CHAR* globalIdxCLName,
                                           utilCLUniqueID globalIdxCLUniqID ) ;

      protected:
         virtual INT32   _init( const CHAR *objdata ) ;
         virtual void    _toBson( BSONObjBuilder &builder ) ;

      private:
         BSONObj _indexDef ;
         INT32   _sortBufferSize ;
   } ;
   typedef _clsCreateIdxTask clsCreateIdxTask ;

   class _clsDropIdxTask : public _clsIdxTask
   {
      public:
         _clsDropIdxTask ( UINT64 taskID ) : _clsIdxTask( taskID )
         {
            _taskType = CLS_TASK_DROP_IDX ;
         }
         virtual ~_clsDropIdxTask() {}

         INT32 initTask( const CHAR *clFullName,
                         utilCLUniqueID clUniqID,
                         const CHAR *indexName,
                         const vector<string> &groupList,
                         UINT64 mainTaskID = CLS_INVALID_TASKID ) ;

         INT32 initMainTask( const CHAR *clFullName,
                             utilCLUniqueID clUniqID,
                             const CHAR *indexName,
                             const ossPoolSet<ossPoolString> &groupList,
                             const ossPoolVector<UINT64> &subTaskList ) ;

         virtual const CHAR* commandName() const ;

         virtual INT32 checkConflictWithExistTask( const _clsTask *pExistTask ) ;

      protected:
         virtual INT32 _init( const CHAR *objdata ) ;
         virtual void  _toBson( BSONObjBuilder &builder ) ;
   } ;
   typedef _clsDropIdxTask clsDropIdxTask ;

   class _clsCopyIdxTask : public _clsIdxTask
   {
      public:
         _clsCopyIdxTask ( UINT64 taskID ) : _clsIdxTask( taskID )
         {
            _taskType = CLS_TASK_COPY_IDX ;
            _isMainTask = TRUE ;
         }
         virtual ~_clsCopyIdxTask() {}

         INT32 initMainTask( const CHAR *clFullName,
                             utilCLUniqueID clUniqID,
                             const ossPoolSet<ossPoolString> &subCLList,
                             const ossPoolSet<ossPoolString> &indexList,
                             const ossPoolSet<ossPoolString> &groupList,
                             const ossPoolVector<UINT64> &subTaskList ) ;

         virtual const CHAR* indexName() const { return NULL ; }
         virtual const CHAR* commandName() const ;
         virtual BOOLEAN muteXOn ( const _clsTask* pOther ) { return FALSE ; }

         const ossPoolSet<ossPoolString>& indexList() const ;

         virtual INT32 checkConflictWithExistTask( const _clsTask *pExistTask ) ;

      protected:
         virtual INT32       _init( const CHAR *objdata ) ;
         virtual void        _toBson( BSONObjBuilder &builder ) ;

      private:
         ossPoolSet<ossPoolString> _subCLList ;
         ossPoolSet<ossPoolString> _indexList ;
   } ;
   typedef _clsCopyIdxTask clsCopyIdxTask ;
}

#endif //CLS_TASK_HPP_

