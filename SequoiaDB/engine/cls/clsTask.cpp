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

#include "clsTask.hpp"
#include "msgCatalogDef.h"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "ixm_common.hpp"
#include "rtn.hpp"

using namespace bson ;

namespace engine
{
   const CHAR* clsTaskTypeStr( CLS_TASK_TYPE taskType )
   {
      switch( taskType )
      {
         case CLS_TASK_SPLIT :
            return VALUE_NAME_SPLIT ;
         case CLS_TASK_SEQUENCE :
            return VALUE_NAME_ALTERSEQUENCE ;
         case CLS_TASK_CREATE_IDX :
            return VALUE_NAME_CREATEIDX ;
         case CLS_TASK_DROP_IDX :
            return VALUE_NAME_DROPIDX ;
         case CLS_TASK_COPY_IDX :
            return VALUE_NAME_COPYIDX ;
         default :
            break ;
      }
      return "Unknown" ;
   }

   const CHAR* clsTaskStatusStr( CLS_TASK_STATUS taskStatus )
   {
      switch( taskStatus )
      {
         case CLS_TASK_STATUS_READY :
            return VALUE_NAME_READY ;
         case CLS_TASK_STATUS_RUN :
            return VALUE_NAME_RUNNING ;
         case CLS_TASK_STATUS_PAUSE :
            return VALUE_NAME_PAUSE ;
         case CLS_TASK_STATUS_CANCELED :
            return VALUE_NAME_CANCELED ;
         case CLS_TASK_STATUS_META :
            return VALUE_NAME_CHGMETA ;
         case CLS_TASK_STATUS_CLEANUP :
            return VALUE_NAME_CLEANUP ;
         case CLS_TASK_STATUS_ROLLBACK :
            return VALUE_NAME_ROLLBACK ;
         case CLS_TASK_STATUS_FINISH :
            return VALUE_NAME_FINISH ;
         case CLS_TASK_STATUS_END :
            return VALUE_NAME_END ;
         default :
            break ;
      }
      return "Unknown" ;
   }

   /*
      _clsTask : implement
   */
   void _clsTask::setStatus( CLS_TASK_STATUS status )
   {
      _status = status ;
   }

   const CHAR* _clsTask::commandName() const
   {
      return NULL ;
   }

   BOOLEAN _clsTask::hasMainTask() const
   {
      return CLS_INVALID_TASKID != _mainTaskID ;
   }

   INT32 _clsTask::getSubTasks( ossPoolVector<UINT64>& list )
   {
      return SDB_OK ;
   }

   INT32 _clsTask::buildStartTask( const BSONObj& obj,
                                   BSONObj& updator,
                                   BSONObj& matcher )
   {
      return SDB_OK ;
   }

   INT32 _clsTask::buildStartTaskBy( const clsTask* pSubTask,
                                     const BSONObj& obj,
                                     BSONObj& updator,
                                     BSONObj& matcher )
   {
      return SDB_OK ;
   }

   INT32 _clsTask::buildCancelTask( const BSONObj& obj,
                                    BSONObj& updator,
                                    BSONObj& matcher )
   {
      return SDB_OK ;
   }

   INT32 _clsTask::buildCancelTaskBy( const clsTask* pSubTask,
                                      BSONObj& updator,
                                      BSONObj& matcher )
   {
      return SDB_OK ;
   }

   INT32 _clsTask::buildReportTask( const BSONObj& obj,
                                    BSONObj& updator,
                                    BSONObj& matcher )
   {
      return SDB_OK ;
   }

   INT32 _clsTask::buildReportTaskBy( const clsTask* pSubTask,
                                      const ossPoolVector<BSONObj>& subTaskInfoList,
                                      BSONObj& updator,
                                      BSONObj& matcher )
   {
      return SDB_OK ;
   }

   INT32 _clsTask::buildQuerySubTasks( const BSONObj& obj,
                                       BSONObj& matcher,
                                       BSONObj& selector )
   {
      return SDB_OK ;
   }

   INT32 _clsTask::buildRemoveTaskBy( UINT64 taskID,
                                      const ossPoolVector<BSONObj>& otherSubTasks,
                                      BSONObj& updator,
                                      BSONObj& matcher )
   {
      return SDB_OK ;
   }

   INT32 _clsTask::toErrInfo( BSONObjBuilder& builder )
   {
      return SDB_OK ;
   }

   INT32 clsNewTask( const BSONObj &taskObj, clsTask*& pTask )
   {
      INT32 rc = SDB_OK ;
      CLS_TASK_TYPE taskType = CLS_TASK_UNKNOWN ;

      // get task type
      rc = rtnGetIntElement( taskObj, FIELD_NAME_TASKTYPE,
                             (INT32&)taskType ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get task type from obj[%s], rc: %d",
                   taskObj.toString().c_str(), rc ) ;

      rc = clsNewTask( taskType, taskObj, pTask ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 clsNewTask( CLS_TASK_TYPE taskType, const BSONObj &taskObj,
                     clsTask*& pTask )
   {
      INT32 rc = SDB_OK ;
      UINT64 taskID = CLS_INVALID_TASKID ;

      SDB_ASSERT( NULL == pTask, "pTask should be null" ) ;

      // get task id
      rc = rtnGetNumberLongElement( taskObj, FIELD_NAME_TASKID,
                                    (INT64&)taskID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Faield to get task ID from obj[%s], rc: %d",
                   taskObj.toString().c_str(), rc ) ;

      // new task
      switch ( taskType )
      {
         case CLS_TASK_SPLIT :
            pTask = SDB_OSS_NEW _clsSplitTask( taskID ) ;
            break ;
         case CLS_TASK_CREATE_IDX :
            pTask = SDB_OSS_NEW _clsCreateIdxTask( taskID ) ;
            break ;
         case CLS_TASK_DROP_IDX :
            pTask = SDB_OSS_NEW _clsDropIdxTask( taskID ) ;
            break ;
         case CLS_TASK_COPY_IDX :
            pTask = SDB_OSS_NEW _clsCopyIdxTask( taskID ) ;
            break ;
         case CLS_TASK_SEQUENCE :
            pTask = SDB_OSS_NEW _clsSequenceTask( taskID ) ;
            break ;
         default :
            PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                         "Unknown task type[%d]",
                         taskType ) ;
      }
      PD_CHECK ( pTask, SDB_OOM, error, PDERROR,
                 "Failed to alloc memory for task[type: %d]",
                 taskType ) ;

      // init task
      rc = pTask->init( taskObj.objdata() ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to init task by obj[%s], rc: %d",
                   taskObj.toString().c_str(), rc ) ;

   done:
      return rc ;
   error:
      if ( pTask )
      {
         SDB_OSS_DEL pTask ;
         pTask = NULL ;
      }
      goto done ;
   }

   void  clsReleaseTask( clsTask*& pTask )
   {
      if ( pTask )
      {
         SDB_OSS_DEL pTask ;
         pTask = NULL ;
      }
   }

   /*
      _clsTaskMgr : implement
   */
   _clsTaskMgr::_clsTaskMgr ( UINT32 maxLocationID )
   {
      _locationID = CLS_INVALID_LOCATIONID ;
      _maxID      = maxLocationID ;
   }

   _clsTaskMgr::~_clsTaskMgr ()
   {
      std::map<UINT32, PAIR_TASK_CNT>::iterator it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         SDB_OSS_DEL it->second.first ;
         ++it ;
      }
      _taskMap.clear() ;
   }

   UINT32 _clsTaskMgr::getLocationID ()
   {
      ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;
      ++_locationID ;
      if ( CLS_INVALID_LOCATIONID == _locationID ||
           ( CLS_INVALID_LOCATIONID != _maxID && _locationID > _maxID )  )
      {
         _locationID = CLS_INVALID_LOCATIONID + 1 ;
      }
      return _locationID ;
   }

   UINT32 _clsTaskMgr::taskCount ()
   {
      ossScopedLock lock ( &_taskLatch, SHARED ) ;
      return (UINT32)_taskMap.size() ;
   }

   UINT32 _clsTaskMgr::taskCount( CLS_TASK_TYPE type )
   {
      UINT32 taskCount = 0 ;

      ossScopedLock lock ( &_taskLatch, SHARED ) ;

      std::map<UINT32, PAIR_TASK_CNT>::iterator it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         clsTask *pTask = it->second.first ;
         if ( type == pTask->taskType() )
         {
            ++taskCount ;
         }
         ++it ;
      }

      return taskCount ;
   }

   UINT32 _clsTaskMgr::taskCountByCL( const CHAR *pCLName )
   {
      UINT32 taskCount = 0 ;

      ossScopedLock lock ( &_taskLatch, SHARED ) ;

      std::map<UINT32, PAIR_TASK_CNT>::iterator it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         clsTask *pTask = it->second.first ;
         if ( pTask->collectionName() &&
              '\0' != pTask->collectionName()[0] &&
              0 == ossStrcmp( pCLName, pTask->collectionName() ) )
         {
            ++taskCount ;
         }
         ++it ;
      }

      return taskCount ;
   }

   UINT32 _clsTaskMgr::taskCountByCS( const CHAR *pCSName )
   {
      UINT32 taskCount = 0 ;

      ossScopedLock lock ( &_taskLatch, SHARED ) ;

      std::map<UINT32, PAIR_TASK_CNT>::iterator it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         clsTask *pTask = it->second.first ;
         if ( pTask->collectionSpaceName() &&
              0 == ossStrcmp( pCSName, pTask->collectionSpaceName() ) )
         {
            ++taskCount ;
         }
         ++it ;
      }

      return taskCount ;
   }

   UINT32 _clsTaskMgr::idxTaskCount()
   {
      UINT32 taskCount = 0 ;

      ossScopedLock lock ( &_taskLatch, SHARED ) ;

      std::map<UINT32, PAIR_TASK_CNT>::iterator it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         clsTask *pTask = it->second.first ;
         if ( CLS_TASK_CREATE_IDX == pTask->taskType() ||
              CLS_TASK_DROP_IDX == pTask->taskType() ||
              CLS_TASK_COPY_IDX == pTask->taskType() )
         {
            ++taskCount ;
         }
         ++it ;
      }

      return taskCount ;
   }

   INT32 _clsTaskMgr::waitTaskEvent( INT64 millisec )
   {
      return _taskEvent.wait( millisec ) ;
   }

   #define CLS_IDXTASK_MAX ( 10 )

   PD_TRACE_DECLARE_FUNCTION ( SDB__CLSTKMGR_ADDTK, "_clsTaskMgr::addTask" )
   INT32 _clsTaskMgr::addTask ( _clsTask *pTask, UINT32 locationID,
                                BOOLEAN *pAlreadyExist )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSTKMGR_ADDTK ) ;
      INT32 referenceCnt = 0 ;
      INT32 idxTaskCnt = 0 ;
      BOOLEAN addIdxTask = FALSE ;

      if ( pAlreadyExist )
      {
         *pAlreadyExist = FALSE ;
      }
      if ( CLS_TASK_CREATE_IDX == pTask->taskType() ||
           CLS_TASK_DROP_IDX   == pTask->taskType() )
      {
         addIdxTask = TRUE ;
      }

      ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;

      // check other tasks in map conflict with the task
      std::map<UINT32, PAIR_TASK_CNT>::iterator it ;
      for ( it = _taskMap.begin() ; it != _taskMap.end() ; it++ )
      {
         _clsTask *tmpTask = it->second.first ;

         if ( locationID == it->first &&
              pTask->taskID() == tmpTask->taskID() )
         {
            // skip same task
            continue ;
         }
         if ( addIdxTask )
         {
            if ( CLS_TASK_CREATE_IDX == tmpTask->taskType() ||
                 CLS_TASK_DROP_IDX   == tmpTask->taskType() )
            {
               idxTaskCnt++ ;
               if ( idxTaskCnt >= CLS_IDXTASK_MAX )
               {
                  rc = SDB_CLS_IDXTASK_EXCEEDED ;
                  PD_LOG( PDWARNING,
                          "The number of index tasks exceeds the maximum: %u",
                          CLS_IDXTASK_MAX ) ;
                  goto error ;
               }
            }
         }
         if ( pTask->muteXOn( tmpTask ) || tmpTask->muteXOn( pTask ) )
         {
            PD_LOG ( PDWARNING,
                     "Exist task[%lld,%s] mutex with new task[%lld,%s]",
                     tmpTask->taskID(), tmpTask->taskName(),
                     pTask->taskID(), pTask->taskName() ) ;
            rc = SDB_CLS_MUTEX_TASK_EXIST ;
            goto error ;
         }
      }

      // if the task already exists in map, just increase reference count,
      // if not, add the task to map
      it = _taskMap.find( locationID ) ;
      if ( it != _taskMap.end() )
      {
         SDB_ASSERT( it->second.first->taskID() == pTask->taskID(),
                     "Unexpected task id" ) ;
         if ( pAlreadyExist )
         {
            *pAlreadyExist = TRUE ;
         }
         referenceCnt = ++(it->second.second) ;
      }
      else
      {
         try
         {
            _taskMap[ locationID ] = std::make_pair( pTask, 1 ) ;
            referenceCnt = 1 ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      PD_LOG( PDDEBUG,
              "Add task: locationID[%u], taskID[%llu], referenceCnt[%d]",
              locationID, pTask->taskID(), referenceCnt ) ;

   done:
      PD_TRACE_EXITRC ( SDB__CLSTKMGR_ADDTK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__CLSTKMGR_RVTK1, "_clsTaskMgr::removeTask" )
   INT32 _clsTaskMgr::removeTask ( UINT32 locationID, BOOLEAN *pDeleted )
   {
      PD_TRACE_ENTRY ( SDB__CLSTKMGR_RVTK1 ) ;

      if ( pDeleted )
      {
         *pDeleted = FALSE ;
      }

      ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;
      std::map<UINT32, PAIR_TASK_CNT>::iterator it =
                                                _taskMap.find ( locationID ) ;
      if ( it != _taskMap.end() )
      {
         INT32 referenceCnt = --(it->second.second) ;
         if ( referenceCnt <= 0 )
         {
            SDB_OSS_DEL it->second.first ;
            if ( pDeleted )
            {
               *pDeleted = TRUE ;
            }
            _taskMap.erase( it ) ;
            _taskEvent.signal() ;
         }

         PD_LOG( PDDEBUG, "Remove task: locationID[%u], referenceCnt[%d]",
                 locationID, referenceCnt ) ;
      }

      PD_TRACE_EXIT ( SDB__CLSTKMGR_RVTK1 ) ;
      return SDB_OK ;
   }

   _clsTask* _clsTaskMgr::findTask ( UINT32 locationID )
   {
      ossScopedLock lock ( &_taskLatch, SHARED ) ;
      std::map<UINT32, PAIR_TASK_CNT>::iterator it =
                                                _taskMap.find ( locationID ) ;
      if ( it != _taskMap.end() )
      {
         return it->second.first ;
      }
      return NULL ;
   }

   void _clsTaskMgr::stopTask( UINT32 locationID )
   {
      ossScopedLock lock ( &_taskLatch, SHARED ) ;
      std::map<UINT32, PAIR_TASK_CNT>::iterator it =
                                                _taskMap.find ( locationID ) ;
      if ( it != _taskMap.end() )
      {
         return it->second.first->setStatus( CLS_TASK_STATUS_CANCELED ) ;
      }
   }

   ossPoolString _clsTaskMgr::dumpTasks( CLS_TASK_TYPE type )
   {
      ossPoolString taskStr ;

      ossScopedLock lock ( &_taskLatch, SHARED ) ;

      std::map<UINT32, PAIR_TASK_CNT>::iterator it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         clsTask *pTask = it->second.first ;
         if ( CLS_TASK_UNKNOWN == type ||
              type == pTask->taskType() )
         {
            CHAR tmp[512] = { 0 } ;
            ossSnprintf( tmp, sizeof( tmp ),
                         "[ taskName: %s, taskID: %llu, localtionID: %u ]",
                         pTask->taskName() ? pTask->taskName() : "",
                         pTask->taskID(), it->first ) ;
            taskStr += tmp ;
         }
         ++it ;
         if ( it != _taskMap.end() )
         {
            taskStr += "\n" ;
         }
      }

      return taskStr ;
   }

   void _clsTaskMgr::regCollection( const string & clName )
   {
      ossScopedLock lock( &_regLatch, EXCLUSIVE ) ;

      std::map<string, UINT32>::iterator it = _mapRegister.find( clName ) ;
      if ( it == _mapRegister.end() )
      {
         _mapRegister[ clName ] = 1 ;
      }
      else
      {
         ++(it->second) ;
      }
   }

   void _clsTaskMgr::unregCollection( const string & clName )
   {
      ossScopedLock lock( &_regLatch, EXCLUSIVE ) ;

      std::map<string, UINT32>::iterator it = _mapRegister.find( clName ) ;
      if ( it != _mapRegister.end() )
      {
         if ( it->second > 1 )
         {
            --(it->second) ;
         }
         else
         {
            _mapRegister.erase( it ) ;
         }
      }
   }

   UINT32 _clsTaskMgr::getRegCount( const string & clName, BOOLEAN noLatch )
   {
      UINT32 count = 0 ;

      if ( !noLatch )
      {
         lockReg( SHARED ) ;
      }

      std::map<string, UINT32>::iterator it = _mapRegister.find( clName ) ;
      if ( it != _mapRegister.end() )
      {
         count = it->second ;
      }

      if ( !noLatch )
      {
         releaseReg( SHARED ) ;
      }

      return count ;
   }

   void _clsTaskMgr::lockReg( OSS_LATCH_MODE mode )
   {
      if ( SHARED == mode )
      {
         _regLatch.get_shared() ;
      }
      else
      {
         _regLatch.get() ;
      }
   }

   void _clsTaskMgr::releaseReg( OSS_LATCH_MODE mode )
   {
      if ( SHARED == mode )
      {
         _regLatch.release_shared() ;
      }
      else
      {
         _regLatch.release() ;
      }
   }

   /*
      _clsDummyTask : implement
   */
   _clsDummyTask::_clsDummyTask( UINT64 taskID )
   :_clsTask( taskID )
   {
   }

   _clsDummyTask::~_clsDummyTask()
   {
   }

   const CHAR* _clsDummyTask::taskName() const
   {
      return "DummyTask" ;
   }

   BOOLEAN _clsDummyTask::muteXOn ( const _clsTask *pOther )
   {
      return FALSE ;
   }

   const CHAR* _clsDummyTask::collectionName() const
   {
      return "" ;
   }

   const CHAR* _clsDummyTask::collectionSpaceName() const
   {
      return "" ;
   }

   INT32 _clsDummyTask::init( const CHAR* objdata )
   {
      return SDB_OK ;
   }

   BSONObj _clsDummyTask::toBson ( UINT32 mask )
   {
      return BSONObj() ;
   }

   /*
      _clsSequenceTask : implement
   */
   _clsSequenceTask::_clsSequenceTask( UINT64 taskID )
   :_clsTask( taskID )
   {
      _taskType = CLS_TASK_SEQUENCE ;
      _status = CLS_TASK_STATUS_FINISH ;
   }

   _clsSequenceTask::~_clsSequenceTask()
   {
   }

   const CHAR* _clsSequenceTask::taskName() const
   {
      return "SequenceTask" ;
   }

   BOOLEAN _clsSequenceTask::muteXOn ( const _clsTask *pOther )
   {
      return FALSE ;
   }

   const CHAR* _clsSequenceTask::collectionName() const
   {
      return "" ;
   }

   const CHAR* _clsSequenceTask::collectionSpaceName() const
   {
      return "" ;
   }

   INT32 _clsSequenceTask::init( const CHAR* objdata )
   {
      return SDB_OK ;
   }

   BSONObj _clsSequenceTask::toBson ( UINT32 mask )
   {
      return BSONObj() ;
   }

   /*
      _clsSplitTask : implement
   */
   _clsSplitTask::_clsSplitTask ( UINT64 taskID )
   : _clsTask ( taskID )
   {
      _sourceID = 0 ;
      _dstID = 0 ;
      _clUniqueID = UTIL_UNIQUEID_NULL ;
      _taskType = CLS_TASK_SPLIT ;
      _status = CLS_TASK_STATUS_READY ;
      _percent  = 0.0 ;
      _resultCode = SDB_OK ;
      //_lockEnd = FALSE ;
   }

   _clsSplitTask::~_clsSplitTask ()
   {
   }

   void _clsSplitTask::_makeName ()
   {
      _taskName = "Split-" ;
      _taskName += _clFullName ;
      _taskName += "{ Begin:" ;
      _taskName += _splitKeyObj.toString() ;
      _taskName += ", End:" ;
      _taskName += _splitEndKeyObj.toString() ;
      _taskName += " } " ;

      /// cs name make
      size_t npos = _clFullName.find( '.' ) ;
      _csName = _clFullName.substr( 0, npos ) ;
   }

   INT32 _clsSplitTask::init ( const CHAR * clFullName, INT32 sourceID,
                               const CHAR * sourceName, INT32 dstID,
                               const CHAR * dstName, const BSONObj & bKey,
                               const BSONObj & eKey, FLOAT64 percent,
                               clsCatalogSet &cataSet )
   {
      INT32 rc = SDB_OK ;
      //_lockEnd = FALSE ;

      _clFullName    = clFullName ;
      _clUniqueID    = cataSet.clUniqueID() ;
      _sourceID      = sourceID ;
      _sourceName    = sourceName ;
      _dstID         = dstID ;
      _dstName       = dstName ;
      _splitKeyObj   = bKey.getOwned() ;
      _splitEndKeyObj= eKey.getOwned() ;
      _shardingKey   = cataSet.getShardingKey().getOwned() ;
      _percent       = percent ;
      _shardingType  = CAT_SHARDING_TYPE_RANGE ;
      if ( cataSet.isHashSharding() )
      {
         _shardingType = CAT_SHARDING_TYPE_HASH ;
      }

      // calc the end key
      BSONObj groupUpBound ;
      BSONObj allUpbound ;
      rc = cataSet.getGroupUpBound( sourceID, groupUpBound ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get group up bound, rc: %d", rc ) ;

      rc = cataSet.getGroupUpBound( 0, allUpbound ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get all up bound, rc: %d", rc ) ;

      // bKey can't empty
      PD_CHECK( !_splitKeyObj.isEmpty(), SDB_INVALIDARG, error, PDERROR,
                "Split begin key can't be empty" ) ;

      // check begin valid
      if ( cataSet.isHashSharding() )
      {
         PD_CHECK( bKey.firstElement().numberInt() <
                   groupUpBound.firstElement().numberInt() &&
                   bKey.firstElement().numberInt() >= 0, SDB_INVALIDARG,
                   error, PDERROR, "Init split task failed, catalog info: %s, "
                   "source group id: %d, bKey: %s",
                   cataSet.toCataInfoBson().toString().c_str(), sourceID,
                   bKey.toString().c_str() ) ;
      }
      else
      {
         PD_CHECK( bKey.woCompare( groupUpBound, _shardingKey, false ) < 0,
                   SDB_INVALIDARG, error, PDERROR, "Init split task failed, "
                   "catalog info : %s, source group id: %d, bKey: %s",
                   cataSet.toCataInfoBson().toString().c_str(), sourceID,
                   bKey.toString().c_str() ) ;
      }

      // calc eKey
      if ( _splitEndKeyObj.isEmpty() )
      {
         _splitEndKeyObj = groupUpBound.getOwned() ;
      }

      if ( 0 == _splitEndKeyObj.woCompare( allUpbound, BSONObj(), false ) )
      {
         _splitEndKeyObj = BSONObj() ;
      }

      // make sure eKey > bKey
      if ( !_splitEndKeyObj.isEmpty() )
      {
         if ( cataSet.isHashSharding() )
         {
            PD_CHECK( _splitKeyObj.firstElement().numberInt() <
                      _splitEndKeyObj.firstElement().numberInt(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Split begin key must less than end key" ) ;
         }
         else
         {
            PD_CHECK( _splitKeyObj.woCompare( _splitEndKeyObj, _shardingKey,
                      false ) < 0, SDB_INVALIDARG, error, PDERROR,
                      "Split begin key must less than end key" ) ;
         }

         /*if ( !isHashSharding() &&
              0 == _splitEndKeyObj.woCompare( groupUpBound, BSONObj(), false ) )
         {
            _lockEnd = TRUE ;
         }*/
      }

      _makeName () ;

   done:
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLITTK_INIT, "_clsSplitTask::init" )
   INT32 _clsSplitTask::init ( const CHAR * objdata )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSPLITTK_INIT ) ;

      if ( !objdata )
      {
         rc = SDB_INVALIDARG ;
         goto done ;
      }

      try
      {
         BSONObj jobObj ( objdata ) ;
         PD_LOG ( PDDEBUG, "Split job: %s", jobObj.toString().c_str() ) ;

         BSONElement ele = jobObj.getField ( CAT_TASKTYPE_NAME ) ;
         PD_CHECK ( ele.type() == NumberInt, SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]", CAT_TASKTYPE_NAME,
                    jobObj.toString().c_str() ) ;
         _taskType = ( CLS_TASK_TYPE )ele.numberInt () ;

         ele = jobObj.getField ( CAT_SPLITPERCENT_NAME ) ;
         PD_CHECK ( ele.isNumber() || ele.eoo(), SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]", CAT_SPLITPERCENT_NAME,
                    jobObj.toString().c_str() ) ;
         _percent = ele.numberDouble() ;

         ele = jobObj.getField ( CAT_STATUS_NAME ) ;
         PD_CHECK ( ele.type() == NumberInt, SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]", CAT_STATUS_NAME,
                    jobObj.toString().c_str() ) ;
         _status = ( CLS_TASK_STATUS )ele.numberInt () ;

         ele = jobObj.getField ( FIELD_NAME_RESULTCODE ) ;
         PD_CHECK ( ele.type() == NumberInt || ele.eoo(), SDB_INVALIDARG, error,
                    PDERROR, "Field[%s] invalid in split task[%s]",
                    FIELD_NAME_RESULTCODE, jobObj.toString().c_str() ) ;
         _resultCode = ele.numberInt () ;

         ele = jobObj.getField ( FIELD_NAME_ENDTIMESTAMP ) ;
         PD_CHECK ( ele.type() == String || ele.eoo(), SDB_INVALIDARG, error,
                    PDERROR, "Field[%s] invalid in task[%s]",
                    FIELD_NAME_ENDTIMESTAMP, jobObj.toString().c_str() ) ;
         ossStringToTimestamp( ele.valuestr(), _endTS ) ;

         ele = jobObj.getField ( CAT_COLLECTION_NAME ) ;
         PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]",
                    CAT_COLLECTION_NAME, jobObj.toString().c_str() ) ;
         _clFullName = ele.str() ;

         ele = jobObj.getField ( CAT_TARGET_NAME ) ;
         PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]", CAT_TARGET_NAME,
                    jobObj.toString().c_str() ) ;
         _dstName = ele.str() ;

         ele = jobObj.getField ( CAT_SOURCE_NAME ) ;
         PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]", CAT_SOURCE_NAME,
                    jobObj.toString().c_str() ) ;
         _sourceName = ele.str() ;

         ele = jobObj.getField ( CAT_SOURCEID_NAME ) ;
         PD_CHECK ( ele.type() == NumberInt, SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]", CAT_SOURCEID_NAME,
                    jobObj.toString().c_str() ) ;
         _sourceID = ele.numberInt () ;

         ele = jobObj.getField ( CAT_TARGETID_NAME ) ;
         PD_CHECK ( ele.type() == NumberInt, SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]", CAT_TARGETID_NAME,
                    jobObj.toString().c_str() ) ;
         _dstID = ele.numberInt () ;

         ele = jobObj.getField ( CAT_SPLITVALUE_NAME ) ;
         PD_CHECK ( ele.type() == Object, SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]", CAT_SPLITVALUE_NAME,
                    jobObj.toString().c_str() ) ;
         _splitKeyObj = ele.embeddedObject().getOwned () ;

         ele = jobObj.getField( CAT_SPLITENDVALUE_NAME ) ;
         PD_CHECK( ele.type() == Object, SDB_INVALIDARG, error, PDERROR,
                   "Field[%s] invalid in split task[%s]",
                   CAT_SPLITENDVALUE_NAME, jobObj.toString().c_str() ) ;
         _splitEndKeyObj = ele.embeddedObject().getOwned() ;

         ele = jobObj.getField( CAT_SHARDINGKEY_NAME ) ;
         PD_CHECK( ele.type() == Object, SDB_INVALIDARG, error, PDERROR,
                   "Field[%s] invalid in split task[%s]", CAT_SHARDINGKEY_NAME,
                   jobObj.toString().c_str() ) ;
         _shardingKey = ele.embeddedObject().getOwned() ;

         ele = jobObj.getField( CAT_SHARDING_TYPE ) ;
         PD_CHECK( ele.type() == String, SDB_INVALIDARG, error, PDERROR,
                   "Field[%s] invalid in split task[%s]", CAT_SHARDING_TYPE,
                   jobObj.toString().c_str() ) ;
         _shardingType = ele.str() ;

         /*ele = jobObj.getField( CLS_SPLIT_TASK_LOCK_END ) ;
         if ( ele.eoo() )
         {
            _lockEnd = FALSE ;
         }
         else
         {
            PD_CHECK( ele.type() == NumberInt, SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] invalid in split task[%s]",
                      CLS_SPLIT_TASK_LOCK_END, jobObj.toString().c_str() ) ;
            _lockEnd = ele.numberInt() ;
         }*/

         ele = jobObj.getField( CAT_CL_UNIQUEID ) ;
         if ( ele.eoo() )
         {
            _clUniqueID = UTIL_UNIQUEID_NULL ;
         }
         else
         {
            PD_CHECK( ele.type() == NumberLong, SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] invalid in split task[%s]",
                      CAT_CL_UNIQUEID, jobObj.toString().c_str() ) ;
            _clUniqueID = (utilCLUniqueID)ele.numberLong() ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Init split task occur expection: %s", e.what () ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _makeName() ;

   done:
      PD_TRACE_EXITRC ( SDB__CLSSPLITTK_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BSONObj _clsSplitTask::toBson( UINT32 mask )
   {
      BSONObjBuilder builder ;

      if ( mask & CLS_SPLIT_MASK_ID )
      {
         builder.append( CAT_TASKID_NAME, (INT64)_taskID ) ;
      }
      if ( mask & CLS_SPLIT_MASK_TYPE )
      {
         builder.append( CAT_TASKTYPE_NAME, (INT32)_taskType ) ;
         builder.append( FIELD_NAME_TASKTYPEDESC, clsTaskTypeStr(_taskType) ) ;
      }
      if ( mask & CLS_SPLIT_MASK_STATUS )
      {
         builder.append( CAT_STATUS_NAME, (INT32)_status ) ;
         builder.append( FIELD_NAME_STATUSDESC, clsTaskStatusStr(_status) ) ;
      }
      if ( mask & CLS_SPLIT_MASK_RESULTCODE )
      {
         builder.append( FIELD_NAME_RESULTCODE, _resultCode ) ;
         builder.append( FIELD_NAME_RESULTCODEDESC,
                         CLS_TASK_STATUS_FINISH == _status ?
                         getErrDesp( _resultCode ) : "" ) ;
      }
      if ( mask & CLS_SPLIT_MASK_CLNAME )
      {
         builder.append( CAT_COLLECTION_NAME, _clFullName ) ;
      }
      if ( mask & CLS_SPLIT_MASK_UNIQUEID )
      {
         builder.append( CAT_CL_UNIQUEID, (INT64)_clUniqueID ) ;
      }
      if ( mask & CLS_SPLIT_MASK_SOURCEID )
      {
         builder.append( CAT_SOURCEID_NAME, _sourceID ) ;
      }
      if ( mask & CLS_SPLIT_MASK_SOURCENAME )
      {
         builder.append( CAT_SOURCE_NAME, _sourceName ) ;
      }
      if ( mask & CLS_SPLIT_MASK_DSTID )
      {
         builder.append( CAT_TARGETID_NAME, _dstID ) ;
      }
      if ( mask & CLS_SPLIT_MASK_DSTNAME )
      {
         builder.append( CAT_TARGET_NAME, _dstName ) ;
      }
      if ( mask & CLS_SPLIT_MASK_BKEY )
      {
         builder.append( CAT_SPLITVALUE_NAME, _splitKeyObj ) ;
      }
      if ( mask & CLS_SPLIT_MASK_EKEY )
      {
         builder.append( CAT_SPLITENDVALUE_NAME, _splitEndKeyObj ) ;
      }
      if ( mask & CLS_SPLIT_MASK_SHARDINGKEY )
      {
         builder.append( CAT_SHARDINGKEY_NAME, _shardingKey ) ;
      }
      if ( mask & CLS_SPLIT_MASK_SHARDINGTYPE )
      {
         builder.append( CAT_SHARDING_TYPE, _shardingType ) ;
      }
      if ( mask & CLS_SPLIT_MASK_PERCENT )
      {
         builder.append( CAT_SPLITPERCENT_NAME, _percent ) ;
      }
      /*if ( mask & CLS_SPLIT_MASK_LOCKEND )
      {
         builder.append( CLS_SPLIT_TASK_LOCK_END, _lockEnd ) ;
      }*/
      CHAR timeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      if ( mask & CLS_SPLIT_MASK_ENDTIMESTAMP )
      {
         if ( CLS_TASK_STATUS_FINISH == _status )
         {
            ossTimestampToString( _endTS, timeStr ) ;
         }
         builder.append( FIELD_NAME_ENDTIMESTAMP, timeStr ) ;
      }

      return builder.obj() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSSPLITTASK_BSTARTTASK, "_clsSplitTask::buildStartTask" )
   INT32 _clsSplitTask::buildStartTask( const BSONObj& obj,
                                        BSONObj& updator,
                                        BSONObj& matcher )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CLSSPLITTASK_BSTARTTASK ) ;

      try
      {
         if ( CLS_TASK_STATUS_READY == _status ||
              CLS_TASK_STATUS_PAUSE == _status )
         {
            matcher = BSON( FIELD_NAME_TASKID << (INT64)_taskID ) ;
            updator = BSON( "$set" <<
                            BSON( FIELD_NAME_STATUS << CLS_TASK_STATUS_RUN <<
                                  FIELD_NAME_STATUSDESC << VALUE_NAME_RUNNING ) ) ;
         }
         else if ( CLS_TASK_STATUS_CANCELED == _status )
         {
            // do not update, let the finial finish request to udpate
            rc = SDB_TASK_HAS_CANCELED ;
            goto error ;
         }
         else if ( CLS_TASK_STATUS_FINISH == _status )
         {
            // already finish, no need to update
            rc = SDB_TASK_ALREADY_FINISHED ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CLSSPLITTASK_BSTARTTASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSSPLITTASK_BCANCELTASK, "_clsSplitTask::buildCancelTask" )
   INT32 _clsSplitTask::buildCancelTask( const BSONObj& obj,
                                         BSONObj& updator,
                                         BSONObj& matcher )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CLSSPLITTASK_BCANCELTASK ) ;

      try
      {
         // can't cancel finish status
         if ( CLS_TASK_STATUS_META == _status ||
              CLS_TASK_STATUS_CLEANUP == _status )
         {
            PD_LOG_MSG( PDERROR, "The task[%llu] status is %s, "
                        "cannot be canceled", _taskID, clsTaskStatusStr( _status ) ) ;
            rc = SDB_TASK_CANNOT_CANCEL ;
            goto error ;
         }
         else if ( CLS_TASK_STATUS_FINISH == _status )
         {
            PD_LOG_MSG( PDERROR, "The task[%llu] is already finished"
                        , _taskID ) ;
            rc = SDB_TASK_ALREADY_FINISHED ;
            goto error ;
         }
         else if ( CLS_TASK_STATUS_READY == _status )
         {
            matcher = BSON( FIELD_NAME_TASKID << (INT64)_taskID ) ;
            updator = BSON( "$set" <<
                            BSON( FIELD_NAME_STATUS << CLS_TASK_STATUS_FINISH <<
                                  FIELD_NAME_STATUSDESC << VALUE_NAME_FINISH <<
                                  FIELD_NAME_RESULTCODE << SDB_TASK_HAS_CANCELED <<
                                  FIELD_NAME_RESULTCODEDESC <<
                                  getErrDesp(SDB_TASK_HAS_CANCELED) ) ) ;
         }
         else if ( CLS_TASK_STATUS_CANCELED != _status )
         {
            matcher = BSON( FIELD_NAME_TASKID << (INT64)_taskID ) ;
            updator = BSON( "$set" <<
                            BSON( FIELD_NAME_STATUS << CLS_TASK_STATUS_CANCELED <<
                                  FIELD_NAME_STATUSDESC << VALUE_NAME_CANCELED ) ) ;

            goto error ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CLSSPLITTASK_BCANCELTASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsSplitTask::calcHashPartition( clsCatalogSet & cataSet,
                                           INT32 groupID, FLOAT64 percent,
                                           BSONObj & bKey, BSONObj & eKey )
   {
      INT32 rc = SDB_OK ;
      INT32 totalNum = 0 ;
      INT32 rangeNum = 0 ;
      INT32 splitNum = 0 ;
      clsCatalogItem *cataItem = NULL ;
      clsCatalogSet::POSITION pos ;

      if ( !cataSet.isHashSharding() )
      {
         rc = SDB_CLS_SHARDING_NOT_HASH ;
         goto error ;
      }

      // calc all partition number
      pos = cataSet.getFirstItem() ;
      while ( NULL != ( cataItem = cataSet.getNextItem( pos ) ) )
      {
         if ( cataItem->getGroupID() != (UINT32)groupID )
         {
            continue ;
         }
         totalNum += ( cataItem->getUpBound().firstElement().numberInt() -
                       cataItem->getLowBound().firstElement().numberInt() ) ;
      }

      PD_CHECK( totalNum > 0, SDB_SYS, error, PDERROR,
                "Catalog Info[%s] error, group id: %d",
                cataSet.toCataInfoBson().toString().c_str(), groupID ) ;

      splitNum = totalNum * ( percent / 100.0 ) ;

      PD_CHECK( splitNum > 0 && splitNum <= totalNum,
                SDB_CLS_SPLIT_PERCENT_LOWER, error, PDERROR,
                "Calc hash split failed, catalog info: %s, group id: %d, "
                "split num: %d, total num: %d, percent: %f",
                cataSet.toCataInfoBson().toString().c_str(), groupID,
                splitNum, totalNum, percent ) ;

      // find the begin key
      splitNum = totalNum - splitNum ;
      pos = cataSet.getFirstItem() ;
      while ( NULL != ( cataItem = cataSet.getNextItem( pos ) ) )
      {
         if ( cataItem->getGroupID() != (UINT32)groupID )
         {
            continue ;
         }
         rangeNum =  cataItem->getUpBound().firstElement().numberInt() -
                     cataItem->getLowBound().firstElement().numberInt() ;

         if ( rangeNum <= splitNum )
         {
            splitNum -= rangeNum ;
         }
         else
         {
            INT32 rangeBegin =
               cataItem->getLowBound().firstElement().numberInt() ;
            bKey = BSON( "" << ( splitNum + rangeBegin ) ) ;
            break ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _clsSplitTask::taskName () const
   {
      return _taskName.c_str() ;
   }

   const CHAR* _clsSplitTask::collectionName() const
   {
      return _clFullName.c_str() ;
   }

   const CHAR* _clsSplitTask::collectionSpaceName() const
   {
      return _csName.c_str() ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLITTK_MXON, "_clsSplitTask::muteXOn" )
   BOOLEAN _clsSplitTask::muteXOn ( const _clsTask * pOther )
   {
      PD_TRACE_ENTRY ( SDB__CLSSPLITTK_MXON ) ;
      BOOLEAN ret = FALSE ;
      if ( taskType() != pOther->taskType() )
      {
         goto done ;
      }
      {
         _clsSplitTask *pOtherSplit = ( _clsSplitTask*)pOther ;

         if ( 0 != ossStrcmp ( collectionName (), pOtherSplit->collectionName () ) )
         {
            goto done ;
         }
         /*else if ( ( splitEndKeyObj().isEmpty() ||
                     pOtherSplit->splitEndKeyObj().isEmpty() ) &&
                   ( sourceID() == pOtherSplit->sourceID() ||
                     sourceID() == pOtherSplit->dstID() ||
                     dstID() == pOtherSplit->sourceID() ||
                     dstID() == pOtherSplit->dstID() ) )
         {
            ret = TRUE ;
            goto done ;
         }*/
         /*else if ( pOtherSplit->dstID() == sourceID() )
         {
            ret = TRUE ;
            goto done ;
         }*/
         else
         {
            INT32 beginResult = splitKeyObj().woCompare(
                                              pOtherSplit->splitKeyObj(),
                                              _getOrdering(), false ) ;
            if ( 0 == beginResult )
            {
               ret = TRUE ;
               goto done ;
            }
            else if ( beginResult < 0 &&
                      ( splitEndKeyObj().woCompare( pOtherSplit->splitKeyObj(),
                                                 _getOrdering(), false ) > 0 ||
                        splitEndKeyObj().isEmpty() ) )
            {
               ret = TRUE ;
               goto done ;
            }
            else if ( beginResult > 0 &&
                      ( splitKeyObj().woCompare( pOtherSplit->splitEndKeyObj(),
                                                 _getOrdering(), false ) < 0 ||
                        pOtherSplit->splitEndKeyObj().isEmpty() ) )
            {
               ret = TRUE ;
               goto done ;
            }
            // lock end
            /*else if ( _lockEnd && beginResult < 0 )
            {
               ret = TRUE ;
               goto done ;
            }*/
         }
      }
   done :
      PD_TRACE_EXIT ( SDB__CLSSPLITTK_MXON ) ;
      return ret ;
   }

   utilCLUniqueID _clsSplitTask::clUniqueID () const
   {
      return _clUniqueID ;
   }

   const CHAR* _clsSplitTask::shardingType () const
   {
      return _shardingType.c_str() ;
   }

   const CHAR* _clsSplitTask::sourceName () const
   {
      return _sourceName.c_str() ;
   }

   const CHAR* _clsSplitTask::dstName () const
   {
      return _dstName.c_str() ;
   }

   UINT32 _clsSplitTask::sourceID () const
   {
      return _sourceID ;
   }

   UINT32 _clsSplitTask::dstID () const
   {
      return _dstID ;
   }

   BSONObj _clsSplitTask::splitKeyObj () const
   {
      return _splitKeyObj ;
   }

   BSONObj _clsSplitTask::splitEndKeyObj () const
   {
      return _splitEndKeyObj ;
   }

   BSONObj _clsSplitTask::shardingKey () const
   {
      return _shardingKey ;
   }

   BOOLEAN _clsSplitTask::isHashSharding () const
   {
      if ( 0 == ossStrcmp( shardingType(), FIELD_NAME_SHARDTYPE_HASH ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BSONObj _clsSplitTask::_getOrdering () const
   {
      if ( !isHashSharding () )
      {
         return _shardingKey ;
      }
      return BSONObj() ;
   }

   /*
      _clsSubTaskUnit : implement
   */
   INT32 _clsSubTaskUnit::init( const CHAR* objdata )
   {
      INT32 rc = SDB_OK ;
      BSONElement e ;

      if ( !objdata )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         BSONObj obj ( objdata ) ;
         BSONObjIterator i( obj ) ;
         while ( i.more() )
         {
            BSONElement e = i.next();

            if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_TASKID ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_TASKID, obj.toString().c_str() ) ;
               taskID = (UINT64)e.numberLong() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_TASKTYPE ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_TASKTYPE, obj.toString().c_str() ) ;
               taskType = (CLS_TASK_TYPE)e.numberInt() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_STATUS ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_STATUS, obj.toString().c_str() ) ;
               status = (CLS_TASK_STATUS)e.numberInt() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_RESULTCODE ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_RESULTCODE, obj.toString().c_str() ) ;
               resultCode = e.numberInt() ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BSONObj _clsSubTaskUnit::toBson()
   {
      BSONObjBuilder b ;
      b.append( FIELD_NAME_TASKID,     (INT64)taskID ) ;
      b.append( FIELD_NAME_TASKTYPE,   (INT32)taskType ) ;
      b.append( FIELD_NAME_STATUS,     (INT32)status ) ;
      b.append( FIELD_NAME_STATUSDESC, clsTaskStatusStr(status) ) ;
      b.append( FIELD_NAME_RESULTCODE, resultCode ) ;
      return b.obj() ;
   }

   /*
      _clsIdxTaskGroupUnit : implement
   */
   INT32 _clsIdxTaskGroupUnit::init( const CHAR* objdata )
   {
      INT32 rc = SDB_OK ;
      BSONElement e ;

      if ( !objdata )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         BSONObj obj( objdata ) ;
         BSONObjIterator i( obj ) ;
         while ( i.more() )
         {
            BSONElement e = i.next();

            if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_GROUPNAME ) )
            {
               PD_CHECK ( e.type() == String, SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_GROUPNAME, obj.toString().c_str() ) ;
               groupName = e.valuestr() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_TASKID ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_TASKID, obj.toString().c_str() ) ;
               taskID = (UINT64)e.numberLong() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_STATUS ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_STATUS, obj.toString().c_str() ) ;
               status = (CLS_TASK_STATUS)e.numberInt() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_PROGRESS ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_PROGRESS, obj.toString().c_str() ) ;
               progress = (UINT32)e.numberInt() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_SPEED ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_SPEED, obj.toString().c_str() ) ;
               speed = (UINT64)e.numberLong() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_TIMESPENT ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_TIMESPENT, obj.toString().c_str() ) ;
               timeSpent = e.numberDouble() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_TIMELEFT ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_TIMELEFT, obj.toString().c_str() ) ;
               timeLeft = e.numberDouble() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_RESULTCODE ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_RESULTCODE, obj.toString().c_str() ) ;
               resultCode = e.numberInt() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_RESULTINFO ) )
            {
               PD_CHECK ( e.type() == Object, SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_RESULTINFO, obj.toString().c_str() ) ;
               resultInfo = e.embeddedObject() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_OPINFO ) )
            {
               PD_CHECK ( e.type() == String, SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_OPINFO, obj.toString().c_str() ) ;
               opInfo = e.valuestr() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_RETRY_COUNT ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_RETRY_COUNT, obj.toString().c_str() ) ;
               retryCnt = e.numberInt() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_TOTAL_RECORDS ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_TOTAL_RECORDS, obj.toString().c_str() ) ;
               totalRecNum = (UINT64)e.numberLong() ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(),
                                      FIELD_NAME_PROCESSED_RECORDS ) )
            {
               PD_CHECK ( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                          "Field[%s] invalid in task[%s]",
                          FIELD_NAME_PROCESSED_RECORDS, obj.toString().c_str() ) ;
               pcsedRecNum = (UINT64)e.numberLong() ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BSONObj _clsIdxTaskGroupUnit::toBson()
   {
      BSONObjBuilder b ;
      b.append( FIELD_NAME_GROUPNAME,      groupName.c_str() ) ;
      b.append( FIELD_NAME_STATUS,         status ) ;
      b.append( FIELD_NAME_STATUSDESC,     clsTaskStatusStr(status) ) ;
      b.append( FIELD_NAME_RESULTCODE,     resultCode ) ;
      b.append( FIELD_NAME_RESULTCODEDESC, CLS_TASK_STATUS_FINISH == status ?
                                           getErrDesp( resultCode ) : "" ) ;
      b.append( FIELD_NAME_RESULTINFO,        resultInfo ) ;
      b.append( FIELD_NAME_OPINFO,            opInfo.c_str() ) ;
      b.append( FIELD_NAME_RETRY_COUNT,       retryCnt ) ;
      b.append( FIELD_NAME_PROGRESS,          progress ) ;
      b.append( FIELD_NAME_SPEED,             (INT64)speed ) ;
      b.append( FIELD_NAME_TIMESPENT,         timeSpent ) ;
      b.append( FIELD_NAME_TIMELEFT,          timeLeft ) ;
      b.append( FIELD_NAME_TOTAL_RECORDS,     (INT64)totalRecNum ) ;
      b.append( FIELD_NAME_PROCESSED_RECORDS, (INT64)pcsedRecNum ) ;
      return b.obj() ;
   }

   #define CLS_TASK_PROGRESS_100 ( 100 )

   /*
      _clsIdxTask : implement
   */
   _clsIdxTask::_clsIdxTask ( UINT64 taskID )
   : _clsTask ( taskID ),
     _progress( 0 ), _speed( 0 ), _timeSpent( 0.0 ), _timeLeft( 0.0 ),
     _totalGroups( 0 ), _succeededGroups( 0 ), _failedGroups( 0 ),
     _totalTasks( 0 ), _succeededTasks( 0 ), _failedTasks( 0 ),
     _changedMask( 0 ), _changedGroupMask( 0 ), _changedSubtaskMask( 0 ),
     _i( 1 ), _pullSubTaskID( CLS_INVALID_TASKID )
   {
      ossMemset( _clFullName, 0, DMS_COLLECTION_FULL_NAME_SZ + 1 ) ;
      ossMemset( _csName,     0, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
      ossMemset( _indexName,  0, IXM_INDEX_NAME_SIZE + 1 ) ;
      ossGetCurrentTime( _createTS ) ;
   }

   void _clsIdxTask::setStatus( CLS_TASK_STATUS status )
   {
      if ( _status == status )
      {
         return ;
      }
      _clsTask::setStatus( status ) ;
      _changedMask |= CLS_IDX_MASK_STATUS ;
   }

   const CHAR* _clsIdxTask::taskName () const
   {
      return _taskName.c_str() ;
   }

   const CHAR* _clsIdxTask::collectionName() const
   {
      return _clFullName ;
   }

   const CHAR* _clsIdxTask::collectionSpaceName() const
   {
      return _csName ;
   }

   const CHAR* _clsIdxTask::indexName() const
   {
      return _indexName ;
   }

   utilCLUniqueID _clsIdxTask::clUniqueID() const
   {
      return _clUniqueID ;
   }

   const BSONObj& _clsIdxTask::resultInfo() const
   {
      return _resultInfo ;
   }

   void _clsIdxTask::setBeginTimestamp()
   {
      ossGetCurrentTime( _beginTS ) ;
      _changedMask |= CLS_IDX_MASK_BEGINTIME ;
   }

   void _clsIdxTask::setEndTimestamp()
   {
      ossGetCurrentTime( _endTS ) ;
      _changedMask |= CLS_IDX_MASK_ENDTIME ;
   }

   void _clsIdxTask::setRun()
   {
      if ( CLS_TASK_STATUS_RUN == _status )
      {
         return ;
      }
      setStatus( CLS_TASK_STATUS_RUN ) ;
      setBeginTimestamp() ;
   }

   void _clsIdxTask::setFinish( INT32 resultCode, const BSONObj &resultInfo )
   {
      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         return ;
      }

      // timestamp
      if ( 0 == _beginTS.time && 0 == _beginTS.microtm )
      {
         setBeginTimestamp() ;
      }
      setEndTimestamp() ;

      // result code may have been set, eg: cancel task
      if ( SDB_OK == _resultCode )
      {
         _resultCode = resultCode ;
         _resultInfo = resultInfo.getOwned() ;
         _changedMask |= CLS_IDX_MASK_RESULT ;
      }

      // progress
      if ( _progress != CLS_TASK_PROGRESS_100 )
      {
         _progress = CLS_TASK_PROGRESS_100 ;
         _changedMask |= CLS_IDX_MASK_PROGRESS ;
      }

      // status
      setStatus( CLS_TASK_STATUS_FINISH ) ;
   }

   void _clsIdxTask::_calculate()
   {
      /*
      * Update TimeSpent / TimeLeft / Speed / Progress field
      */
      UINT64 allRecNum = 0 ;
      UINT64 allPcsNum = 0 ;
      UINT32 timeLeft = 0 ;

      _changedMask |= CLS_IDX_MASK_PROGRESS ;

      // timeLeft, find the largest value
      for( MAP_GROUP_INFO_IT it = _mapGroupInfo.begin() ;
           it != _mapGroupInfo.end() ;
           it++ )
      {
         const _clsIdxTaskGroupUnit& groupUnit = it->second ;
         allRecNum += groupUnit.totalRecNum ;
         allPcsNum += groupUnit.pcsedRecNum ;
         if ( groupUnit.timeLeft > timeLeft )
         {
            timeLeft = groupUnit.timeLeft ;
         }
      }
      _timeLeft = timeLeft ;

      // timeSpent
      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         UINT64 beginMs = _beginTS.time * 1000000 + _beginTS.microtm  ;
         UINT64 endMs = _endTS.time * 1000000 + _endTS.microtm  ;
         _timeSpent = ( endMs - beginMs ) / 1000000.0 ;
      }
      else
      {
         ossTimestamp currentTS ;
         ossGetCurrentTime( currentTS ) ;
         UINT64 beginMs = _beginTS.time * 1000000 + _beginTS.microtm  ;
         UINT64 currentMs = currentTS.time * 1000000 + currentTS.microtm  ;
         _timeSpent = ( currentMs - beginMs ) / 1000000.0 ;
      }

      // progress
      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         _progress = CLS_TASK_PROGRESS_100 ;
      }
      else
      {
         if ( allRecNum != 0 )
         {
            _progress = (FLOAT64)allPcsNum / allRecNum * 100 ;
         }
         else
         {
            _progress = 0 ;
         }
         if ( CLS_TASK_PROGRESS_100 == _progress )
         {
            // the task is not finished yet.
            _progress = 99 ;
         }
      }

      // speed
      if ( _timeSpent != 0 )
      {
         _speed = allPcsNum / _timeSpent ;
      }
      else
      {
         _speed = 0 ;
      }
   }

   INT32 _clsIdxTask::init ( const CHAR *objdata )
   {
      INT32 rc = SDB_OK ;

      if ( !objdata )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {

      BSONObj obj ( objdata ) ;

      BSONObjIterator i( obj ) ;
      while ( i.more() )
      {
         BSONElement ele = i.next();

         // base fields
         if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TASKTYPE ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]", FIELD_NAME_TASKTYPE,
                       obj.toString().c_str() ) ;
            _taskType = (CLS_TASK_TYPE)ele.numberInt() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_STATUS ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]", FIELD_NAME_STATUS,
                       obj.toString().c_str() ) ;
            _status = (CLS_TASK_STATUS)ele.numberInt() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), CAT_COLLECTION_NAME ) )
         {
            PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]", CAT_COLLECTION_NAME,
                       obj.toString().c_str() ) ;
            ossStrncpy( _clFullName, ele.valuestr(),
                        DMS_COLLECTION_FULL_NAME_SZ ) ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_UNIQUEID ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]", FIELD_NAME_STATUS,
                       obj.toString().c_str() ) ;
            _clUniqueID = (utilCLUniqueID)ele.numberLong() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_INDEXNAME ) )
         {
            PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]",
                       FIELD_NAME_INDEXNAME, obj.toString().c_str() ) ;
            ossStrncpy( _indexName, ele.valuestr(), IXM_INDEX_NAME_SIZE ) ;
         }
         // result
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_RESULTCODE ) )
         {
            PD_CHECK ( ele.isNumber() , SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]",
                       FIELD_NAME_RESULTCODE, obj.toString().c_str() ) ;
            _resultCode = ele.numberInt() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_RESULTINFO ) )
         {
            PD_CHECK ( ele.type() == Object, SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]",
                       FIELD_NAME_RESULTINFO, obj.toString().c_str() ) ;
            _resultInfo = ele.embeddedObject().copy() ;
         }
         // main task
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_MAIN_TASKID ) )
         {
            PD_CHECK ( ele.isNumber() , SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]",
                       FIELD_NAME_MAIN_TASKID, obj.toString().c_str() ) ;
            _mainTaskID = (UINT64)ele.numberLong() ;
         }
         // group count
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALGROUP ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_TOTALGROUP, obj.toString().c_str() ) ;
            _totalGroups = (UINT32)ele.numberInt() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_SUCCEEDGROUP ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_SUCCEEDGROUP, obj.toString().c_str() ) ;
            _succeededGroups = (UINT32)ele.numberInt() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_FAILGROUP ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_FAILGROUP, obj.toString().c_str() ) ;
            _failedGroups = (UINT32)ele.numberInt() ;
         }
         // task count
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALSUBTASK ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_TOTALSUBTASK, obj.toString().c_str() ) ;
            _totalTasks = (UINT32)ele.numberInt() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_SUCCEEDSUBTASK ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_SUCCEEDSUBTASK, obj.toString().c_str() ) ;
            _succeededTasks = (UINT32)ele.numberInt() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_FAILSUBTASK ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_FAILSUBTASK, obj.toString().c_str() ) ;
            _failedTasks = (UINT32)ele.numberInt() ;
         }
         // progress info
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_PROGRESS ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]", FIELD_NAME_PROGRESS,
                       obj.toString().c_str() ) ;
            _progress = (UINT32)ele.numberInt() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_SPEED ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]", FIELD_NAME_SPEED,
                       obj.toString().c_str() ) ;
            _speed = (UINT64)ele.numberLong() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TIMESPENT ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]", FIELD_NAME_TIMESPENT,
                       obj.toString().c_str() ) ;
            _timeSpent = ele.numberDouble() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TIMELEFT ) )
         {
            PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in task[%s]", FIELD_NAME_TIMELEFT,
                       obj.toString().c_str() ) ;
            _timeLeft = ele.numberDouble() ;
         }
         // timestamp
         else if ( 0 == ossStrcmp( ele.fieldName(),
                                   FIELD_NAME_CREATETIMESTAMP ) )
         {
            PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_CREATETIMESTAMP, obj.toString().c_str() ) ;
            if ( 0 != ossStrcmp( ele.valuestrsafe(), "" ) )
            {
               ossStringToTimestamp( ele.valuestrsafe(), _createTS ) ;
            }
         }
         else if ( 0 == ossStrcmp( ele.fieldName(),
                                   FIELD_NAME_BEGINTIMESTAMP ) )
         {
            PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_BEGINTIMESTAMP, obj.toString().c_str() ) ;
            if ( 0 != ossStrcmp( ele.valuestrsafe(), "" ) )
            {
               ossStringToTimestamp( ele.valuestrsafe(), _beginTS ) ;
            }
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_ENDTIMESTAMP ) )
         {
            PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_ENDTIMESTAMP, obj.toString().c_str() ) ;
            if ( 0 != ossStrcmp( ele.valuestrsafe(), "" ) )
            {
               ossStringToTimestamp( ele.valuestrsafe(), _endTS ) ;
            }
         }
         // Groups
         else if ( 0 == ossStrcmp( ele.fieldName(), CAT_GROUPS_NAME ) )
         {
            PD_CHECK ( ele.type() == Array, SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       CAT_GROUPS_NAME, obj.toString().c_str() ) ;

            // Groups: [ { GroupName: xxx, Status: xxx, ... }, ... ]
            BSONObjIterator it( ele.embeddedObject() ) ;
            while( it.more() )
            {
               BSONElement gEle = it.next() ;
               PD_CHECK( gEle.type() == Object , SDB_INVALIDARG, error,
                         PDERROR, "Field[%s] invalid in task[%s]",
                         CAT_GROUPS_NAME, obj.toString().c_str() ) ;

               _clsIdxTaskGroupUnit oneGroup ;
               rc = oneGroup.init( gEle.Obj().objdata() ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to init group info, rc: %d",
                            rc ) ;

               _mapGroupInfo[ oneGroup.groupName ] = oneGroup ;
            }
         }
         // SubTasks
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_SUBTASKS ) )
         {
            PD_CHECK ( ele.type() == Array, SDB_INVALIDARG, error,
                       PDERROR, "Field[%s] invalid in task[%s]",
                       FIELD_NAME_SUBTASKS, obj.toString().c_str() ) ;
            _isMainTask = TRUE ;

            // "SubTasks" : [ { TaskID: xx, Status: xx, ... }, ... ]
            BSONObjIterator it( ele.embeddedObject() ) ;
            while( it.more() )
            {
               BSONElement sEle = it.next() ;
               PD_CHECK( sEle.type() == Object, SDB_INVALIDARG, error,
                         PDERROR, "Field invalid in task[%s]",
                         obj.toString().c_str() ) ;

               _clsSubTaskUnit oneSubTask ;
               rc = oneSubTask.init( sEle.Obj().objdata() ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to init sub-task info, rc: %d",
                            rc ) ;

               _mapSubTask[ oneSubTask.taskID ] = oneSubTask ;
            }
         }
      }

      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      rc = _init( objdata ) ;
      if ( rc )
      {
         goto error ;
      }

      _makeName() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   CLS_TASK_STATUS _clsIdxTask::getTaskStatusByGroup( const CHAR* groupName )
   {
      SDB_ASSERT( groupName, "group name can't be NULL" ) ;
      CLS_TASK_STATUS status = CLS_TASK_STATUS_FINISH ;
      MAP_GROUP_INFO::iterator itr ;

      try
      {
         ossPoolString tmpGroupName( groupName ) ;
         itr = _mapGroupInfo.find( tmpGroupName ) ;
         if ( itr != _mapGroupInfo.end() )
         {
            status = itr->second.status ;
         }
      }
      catch ( std::exception &e )
      {
         itr = _mapGroupInfo.begin() ;
         while ( itr != _mapGroupInfo.end() )
         {
            if ( 0 == ossStrcmp( itr->first.c_str(), groupName ) )
            {
               status = itr->second.status ;
               break ;
            }
            ++itr ;
         }
      }

      return status ;
   }

   BSONObj _clsIdxTask::toBson( UINT32 mask )
   {
      BSONObjBuilder builder ;

      SDB_ASSERT( (UINT32)CLS_MASK_ALL == mask, "Invalid mask" ) ;

      try
      {

      // task base
      builder.append( FIELD_NAME_TASKID, (INT64)_taskID ) ;
      if ( hasMainTask() )
      {
         builder.append( FIELD_NAME_MAIN_TASKID, (INT64)_mainTaskID ) ;
      }

      builder.append( FIELD_NAME_TASKTYPE,     (INT32)_taskType ) ;
      builder.append( FIELD_NAME_TASKTYPEDESC, clsTaskTypeStr( _taskType ) ) ;
      builder.append( FIELD_NAME_STATUS,       (INT32)_status ) ;
      builder.append( FIELD_NAME_STATUSDESC,   clsTaskStatusStr( _status ) ) ;

      // collection index
      builder.append( CAT_COLLECTION_NAME, _clFullName ) ;
      builder.append( FIELD_NAME_UNIQUEID, (INT64)_clUniqueID ) ;
      if ( indexName() )
      {
         builder.append( FIELD_NAME_INDEXNAME, indexName() ) ;
      }

      // result
      builder.append( FIELD_NAME_RESULTCODE, _resultCode ) ;
      builder.append( FIELD_NAME_RESULTCODEDESC,
                      CLS_TASK_STATUS_FINISH == _status ?
                      getErrDesp( _resultCode ) : "" ) ;
      builder.append( FIELD_NAME_RESULTINFO, _resultInfo ) ;

      // progress
      builder.append( FIELD_NAME_PROGRESS,  _progress ) ;
      builder.append( FIELD_NAME_SPEED,     (INT64)_speed ) ;
      builder.append( FIELD_NAME_TIMESPENT, _timeSpent ) ;
      builder.append( FIELD_NAME_TIMELEFT,  _timeLeft ) ;

      // groups info
      BSONArrayBuilder arr( builder.subarrayStart( CAT_GROUPS_NAME ) ) ;
      for( MAP_GROUP_INFO_IT it = _mapGroupInfo.begin() ;
           it != _mapGroupInfo.end() ; ++it )
      {
         arr.append( it->second.toBson() ) ;
      }
      arr.done() ;

      builder.append( FIELD_NAME_TOTALGROUP,   _totalGroups ) ;
      builder.append( FIELD_NAME_SUCCEEDGROUP, _succeededGroups ) ;
      builder.append( FIELD_NAME_FAILGROUP,    _failedGroups ) ;

      // sub-task info
      if ( _isMainTask )
      {
         builder.appendBool( FIELD_NAME_IS_MAINTASK, _isMainTask ) ;

         BSONArrayBuilder arr( builder.subarrayStart( FIELD_NAME_SUBTASKS ) ) ;
         for( MAP_SUBTASK_IT it = _mapSubTask.begin() ;
              it != _mapSubTask.end() ; it++ )
         {
            arr.append( it->second.toBson() ) ;
         }
         arr.done() ;

         builder.append( FIELD_NAME_TOTALSUBTASK,   _totalTasks ) ;
         builder.append( FIELD_NAME_SUCCEEDSUBTASK, _succeededTasks ) ;
         builder.append( FIELD_NAME_FAILSUBTASK,    _failedGroups ) ;
      }

      // timestamp
      CHAR timeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      ossTimestampToString( _createTS, timeStr ) ;
      builder.append( FIELD_NAME_CREATETIMESTAMP, timeStr ) ;

      if ( _status >= CLS_TASK_STATUS_RUN )
      {
         ossTimestampToString( _beginTS, timeStr ) ;
      }
      else
      {
         timeStr[0] = 0 ;
      }
      builder.append( FIELD_NAME_BEGINTIMESTAMP, timeStr ) ;

      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         ossTimestampToString( _endTS, timeStr ) ;
      }
      else
      {
         timeStr[0] = 0 ;
      }
      builder.append( FIELD_NAME_ENDTIMESTAMP, timeStr ) ;

      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      _toBson( builder ) ;

      return builder.obj() ;
   }

   void _clsIdxTask::_makeName()
   {
      // set cs name by cl name
      INT32 i = 0 ;
      while ( _clFullName[ i ] && i < DMS_COLLECTION_SPACE_NAME_SZ )
      {
         if ( '.' == _clFullName[ i ] )
         {
            break ;
         }
         _csName[ i ] = _clFullName[ i ] ;
         ++i ;
      }

      // set task name
      if ( CLS_TASK_CREATE_IDX == _taskType )
      {
         _taskName = "Create index-" ;
      }
      else if ( CLS_TASK_DROP_IDX == _taskType )
      {
         _taskName = "Drop index-" ;
      }
      else if ( CLS_TASK_COPY_IDX == _taskType )
      {
         _taskName = "Copy index-" ;
      }
      else
      {
         _taskName = "Index-" ;
      }
      _taskName += _clFullName ;
      _taskName += "-" ;
      _taskName += _indexName ;
   }

   BOOLEAN _clsIdxTask::muteXOn ( const _clsTask* pOther )
   {
      BOOLEAN ret = FALSE ;
      BOOLEAN sameCL = FALSE ;
      _clsIdxTask *pOtherIdx = NULL ;

      if ( pOther->taskType() != CLS_TASK_CREATE_IDX &&
           pOther->taskType() != CLS_TASK_DROP_IDX )
      {
         goto done ;
      }

      pOtherIdx = (clsIdxTask*)pOther ;

      // if they all have valid unique id, then compare by unique id
      if ( UTIL_IS_VALID_CLUNIQUEID( clUniqueID() ) &&
           UTIL_IS_VALID_CLUNIQUEID( pOtherIdx->clUniqueID() ) &&
           clUniqueID() == pOtherIdx->clUniqueID() )
      {
         sameCL = TRUE ;
      }
      else if ( ! UTIL_IS_VALID_CLUNIQUEID( clUniqueID() ) &&
                ! UTIL_IS_VALID_CLUNIQUEID( pOtherIdx->clUniqueID() ) &&
                0 == ossStrcmp( collectionName(),
                                pOtherIdx->collectionName() ) )
      {
         sameCL = TRUE ;
      }

      if ( sameCL && 0 == ossStrcmp( indexName(),
                                     pOtherIdx->indexName() ) )
      {
         ret = TRUE ;
         goto done ;
      }

   done :
      return ret ;
   }

   INT32 _clsIdxTask::countGroup() const
   {
      return _mapGroupInfo.size() ;
   }

   INT32 _clsIdxTask::countSubTask() const
   {
      return _mapSubTask.size() ;
   }

   INT32 _clsIdxTask::buildQuerySubTasks( const BSONObj& obj,
                                          BSONObj& matcher,
                                          BSONObj& selector )
   {
      INT32 rc = SDB_OK ;
      const CHAR* groupName = NULL ;

      try
      {
         if ( !obj.isEmpty() )
         {
            BSONElement ele = obj.getField( FIELD_NAME_GROUPNAME ) ;
            PD_CHECK( ele.type() == String, SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] invalid in obj[%s]",
                      FIELD_NAME_GROUPNAME, obj.toString().c_str() ) ;
            groupName = ele.valuestr() ;

            // matcher : { MainTaskID: 2, "Groups.GroupName": 'db2' }
            matcher = BSON( FIELD_NAME_MAIN_TASKID << (INT64)_taskID <<
                            FIELD_NAME_GROUPS "." FIELD_NAME_GROUPNAME <<
                            groupName ) ;

            // selector: { "Groups": { "$include": 1,
            //                         "$elemMatch": { "GroupName": "db2" } } }
            selector = BSON( FIELD_NAME_GROUPS <<
                             BSON( "$include" << 1 <<
                                   "$elemMatch" << BSON( FIELD_NAME_GROUPNAME <<
                                                         groupName ) ) ) ;
         }
         else
         {
            matcher = BSON( FIELD_NAME_MAIN_TASKID << (INT64)_taskID ) ;
            selector = BSONObj() ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsIdxTask::toErrInfo( BSONObjBuilder& builder )
   {
      INT32 rc = SDB_OK ;

      if ( CLS_TASK_STATUS_FINISH != _status ||
           SDB_OK == _resultCode )
      {
         goto done ;
      }

      try
      {
         // errno and its description
         builder.append( OP_ERRNOFIELD, _resultCode ) ;
         builder.append( OP_ERRDESP_FIELD, getErrDesp( _resultCode ) ) ;

         // detail
         if ( _resultInfo.isEmpty() )
         {
            builder.append( OP_ERR_DETAIL, "" ) ;
         }
         else if ( 0 == ossStrcmp( _resultInfo.firstElementFieldName(),
                                   FIELD_NAME_DETAIL ) )
         {
            builder.append( OP_ERR_DETAIL,
                            _resultInfo.firstElement().valuestrsafe() ) ;
         }
         else
         {
            builder.appendElements( _resultInfo ) ;
            builder.append( OP_ERR_DETAIL, "" ) ;
         }

         // ErrNodes
         BSONArrayBuilder arrB( builder.subarrayStart( FIELD_NAME_ERROR_NODES ) ) ;
         ossPoolMap<ossPoolString, _clsIdxTaskGroupUnit>::const_iterator it ;
         for( it = _mapGroupInfo.begin() ; it != _mapGroupInfo.end() ; ++it )
         {
            const CHAR* groupName = it->first.c_str() ;
            const _clsIdxTaskGroupUnit& groupUnit = it->second ;
            BSONObjBuilder ob1, ob2 ;

            if ( SDB_OK == groupUnit.resultCode ||
                 SDB_TASK_ROLLBACK == groupUnit.resultCode )
            {
               continue ;
            }

            ob1.append( FIELD_NAME_NODE_NAME, "" ) ;
            ob1.append( FIELD_NAME_GROUPNAME, groupName ) ;
            ob1.append( FIELD_NAME_RCFLAG, groupUnit.resultCode ) ;

            // ErrInfo
            ob2.append( OP_ERRNOFIELD, groupUnit.resultCode ) ;
            ob2.append( OP_ERRDESP_FIELD, getErrDesp( groupUnit.resultCode ) ) ;
            if ( groupUnit.resultInfo.isEmpty() )
            {
               ob2.append( OP_ERR_DETAIL, "" ) ;
            }
            else if ( 0 == ossStrcmp( groupUnit.resultInfo.firstElementFieldName(),
                                      FIELD_NAME_DETAIL ) )
            {
               ob2.append( OP_ERR_DETAIL,
                           groupUnit.resultInfo.firstElement().valuestrsafe() ) ;
            }
            else
            {
               ob2.appendElements( groupUnit.resultInfo ) ;
               ob2.append( OP_ERR_DETAIL, "" ) ;
            }
            ob1.append( FIELD_NAME_ERROR_INFO, ob2.done() ) ;

            arrB.append( ob1.obj() ) ;
         }
         arrB.done() ;

         // TaskID
         builder.append( FIELD_NAME_TASKID, (INT64)_taskID ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsIdxTask::getSubTasks( ossPoolVector<UINT64>& list )
   {
      SDB_ASSERT( _isMainTask, "should be main-task" ) ;
      INT32 rc = SDB_OK ;

      for ( MAP_SUBTASK_IT it = _mapSubTask.begin() ;
            it != _mapSubTask.end() ; ++it )
      {
         try
         {
            list.push_back( it->first ) ;
         }
         catch( std::exception &e )
         {
            PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
         }
      }

   done :
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsIdxTask::buildRemoveTaskBy( UINT64 taskID,
                                         const ossPoolVector<BSONObj>& otherSubTasks,
                                         BSONObj& updator,
                                         BSONObj& matcher )
   {
      SDB_ASSERT( _isMainTask, "should be main-task" ) ;

      INT32 rc = SDB_OK ;
      ossPoolMap< ossPoolString, ossPoolVector<BSONObj> > groupInfo ;
      ossPoolMap< ossPoolString, ossPoolVector<BSONObj> >::iterator groupIt ;

      _clearChangedMask() ;

      // erase SubTasks, decrease SucceededSubTasks/FailedSubTasks/TotalSubTasks
      MAP_SUBTASK_IT it = _mapSubTask.find( taskID ) ;
      if ( _mapSubTask.end() == it )
      {
         goto done ;
      }

      if ( CLS_TASK_STATUS_FINISH == it->second.status )
      {
         if ( _isSucceedTask( it->second ) )
         {
            _decSucceededTasks() ;
         }
         else
         {
            _decFailedTasks() ;
         }
      }
      _decTotalTasks() ;

      _mapSubTask.erase( it ) ;
      _changedMask |= CLS_IDX_MASK_PULL_SUBTASK ;
      _pullSubTaskID = taskID ;

      // build Groups
      for( ossPoolVector<BSONObj>::const_iterator it = otherSubTasks.begin() ;
           it != otherSubTasks.end() ; ++it )
      {
         BSONObj array ;
         rc = rtnGetArrayElement( *it, FIELD_NAME_GROUPS, array ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s] from obj[%s]",
                      FIELD_NAME_GROUPS, it->toString().c_str() ) ;

         BSONObjIterator i( array ) ;
         while( i.more() )
         {
            BSONElement e = i.next() ;
            PD_CHECK( e.type() == Object, SDB_INVALIDARG, error,
                      PDERROR, "Field invalid in task[%s]",
                      array.toString().c_str() ) ;

            BSONObj groupObj = e.Obj() ;
            const CHAR* groupName = NULL ;

            rc = rtnGetStringElement( groupObj, FIELD_NAME_GROUPNAME,
                                      &groupName ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s] from obj[%s]",
                         FIELD_NAME_GROUPNAME, groupObj.toString().c_str() ) ;

            groupIt = groupInfo.find( groupName ) ;
            if ( groupInfo.end() == groupIt )
            {
               ossPoolVector<BSONObj> vec ;
               vec.push_back( groupObj ) ;
               groupInfo[groupName] = vec ;
            }
            else
            {
               ossPoolVector<BSONObj> &tmp = groupIt->second ;
               tmp.push_back( groupObj ) ;
            }
         }
      }

      // reset Groups
      _mapGroupInfo.clear() ;
      _changedMask |= CLS_IDX_MASK_GROUPS ;
      _succeededGroups = 0 ;
      _failedGroups = 0 ;
      _totalGroups = 0 ;
      _changedMask |= CLS_IDX_MASK_GROUPCOUNT ;

      // count Groups
      for ( groupIt = groupInfo.begin() ; groupIt != groupInfo.end() ; groupIt++ )
      {
         const ossPoolVector<BSONObj>& groupVec = groupIt->second ;
         clsIdxTaskGroupUnit newGroup ;

         rc = _buildNewGroupInfo( groupVec, newGroup ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build new group info", rc ) ;

         _mapGroupInfo[ newGroup.groupName ] = newGroup ;
         if ( CLS_TASK_STATUS_FINISH == newGroup.status )
         {
            if ( _isSucceedGroup( newGroup ) )
            {
               _incSucceededGroups() ;
            }
            else
            {
               _incFailedGroups() ;
            }
         }
         _incTotalGroups() ;
      }

      // other field
      _updateOtherBySubTaskInfo() ;

      rc = _toChangedObj( matcher, updator ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build changed bson", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSIDXTASK_BMGTGP, "_clsIdxTask::buildMigrateGroup" )
   INT32 _clsIdxTask::buildMigrateGroup( const CHAR* srcGroup,
                                         const CHAR* dstGroup,
                                         BSONObj& updator,
                                         BSONObj& matcher )
   {
      SDB_ASSERT( srcGroup && dstGroup, "group name can't be null" ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CLSIDXTASK_BMGTGP ) ;
      MAP_GROUP_INFO_IT dIt ;
      MAP_GROUP_INFO_IT sIt ;

      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         PD_LOG( PDWARNING, "Task[%llu] has already finished", _taskID ) ;
         goto done ;
      }
      if ( CLS_TASK_DROP_IDX != _taskType )
      {
         goto done ;
      }

      // Split and drop index are concurrent. If target group has completed
      // drop-index task( no index ), while source group hasn't complete the
      // task( has index ). Target group will replay the indexes of source group
      // while splitting, so target group may recreate index. Therefore, we
      // should migrate the (drop-index) tasks of source group to target group.
      dIt = _mapGroupInfo.find( dstGroup ) ;
      sIt = _mapGroupInfo.find( srcGroup ) ;
      if ( dIt != _mapGroupInfo.end() && sIt != _mapGroupInfo.end() )
      {
         _clsIdxTaskGroupUnit* dstGroupUnit = &(dIt->second) ;
         _clsIdxTaskGroupUnit* srcGroupUnit = &(sIt->second) ;
         if ( CLS_TASK_STATUS_FINISH == dstGroupUnit->status &&
              CLS_TASK_STATUS_FINISH != srcGroupUnit->status )
         {
            _clearChangedMask() ;

            // change SucceedGroups/FailedGroups
            if ( _isSucceedGroup( *dstGroupUnit ) )
            {
               _decSucceededGroups() ;
            }
            else
            {
               _decFailedGroups() ;
            }

            // change group status
            dstGroupUnit->status = CLS_TASK_STATUS_READY ;
            _changedGroupMask |= CLS_IDX_MASK_STATUS ;
            _changedMask |= CLS_IDX_MASK_GROUPS ;

            // change other field
            if ( ! _isMainTask )
            {
               _updateOtherByGroupInfo() ;
            }

            // to bson
            rc = _toChangedObj( dstGroupUnit, NULL, matcher, updator ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build changed bson", rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CLSIDXTASK_BMGTGP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSIDXTASK_BADDGROUP, "_clsIdxTask::buildAddGroup" )
   INT32 _clsIdxTask::buildAddGroup( const CHAR* groupName,
                                     BSONObj& updator,
                                     BSONObj& matcher )
   {
      SDB_ASSERT( groupName, "group name can't be null" ) ;
      SDB_ASSERT( !_isMainTask, "can't be used by main-task" ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CLSIDXTASK_BADDGROUP ) ;

      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         PD_LOG( PDWARNING, "Task[%llu] has already finished", _taskID ) ;
         goto done ;
      }
      if ( _mapGroupInfo.find( groupName ) != _mapGroupInfo.end() )
      {
         PD_LOG( PDWARNING, "Group[%s] has already exist in task[%llu]",
                 groupName, _taskID ) ;
         goto done ;
      }

      _clearChangedMask() ;

      // add to Groups, increase TotalGroups
      {
         clsIdxTaskGroupUnit oneGroup ;
         oneGroup.groupName = groupName ;
         _mapGroupInfo[ oneGroup.groupName ] = oneGroup ;
      }
      _changedMask |= CLS_IDX_MASK_PUSH_GROUP ;
      _pushGroupName = groupName ;

      _incTotalGroups() ;

      // change other field
      _updateOtherByGroupInfo() ;

      // to bson
      rc = _toChangedObj( NULL, NULL, matcher, updator ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build changed bson", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CLSIDXTASK_BADDGROUP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSIDXTASK_BADDGROUPBY, "_clsIdxTask::buildAddGroupBy" )
   INT32 _clsIdxTask::buildAddGroupBy( const _clsTask* pSubTask,
                                       const CHAR* groupName,
                                       BSONObj& updator,
                                       BSONObj& matcher )
   {
      SDB_ASSERT( pSubTask, "pSubTask cann't be null" ) ;
      SDB_ASSERT( groupName, "group name can't be null" ) ;
      SDB_ASSERT( _isMainTask, "should be used by main-task" ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CLSIDXTASK_BADDGROUPBY ) ;
      MAP_GROUP_INFO::iterator itGroup ;
      MAP_SUBTASK::iterator itTask ;

      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         PD_LOG( PDWARNING, "Task[%llu] has already finished", _taskID ) ;
         goto done ;
      }

      _clearChangedMask() ;

      /// find out group unit from Groups List
      itGroup = _mapGroupInfo.find( groupName ) ;
      if ( itGroup == _mapGroupInfo.end() )
      {
         clsIdxTaskGroupUnit oneGroup ;
         oneGroup.groupName = groupName ;
         _mapGroupInfo[ oneGroup.groupName ] = oneGroup ;
         _changedMask |= CLS_IDX_MASK_PUSH_GROUP ;
         _pushGroupName = groupName ;
      }
      else
      {
         PD_LOG( PDWARNING, "Group[%s] already exist in task[%llu]",
                 groupName, _taskID ) ;
         goto done ;
      }

      /// find out sub-task in SubTasks list
      itTask = _mapSubTask.find( pSubTask->taskID() ) ;
      if ( itTask != _mapSubTask.end() )
      {
         clsSubTaskUnit& curSubTask = itTask->second ;
         if ( CLS_TASK_STATUS_FINISH != curSubTask.status )
         {
            /// generate new sub-task info
            clsSubTaskUnit newSubTask ;
            newSubTask.taskID     = pSubTask->taskID() ;
            newSubTask.taskType   = pSubTask->taskType() ;
            newSubTask.status     = pSubTask->status() ;
            newSubTask.resultCode = pSubTask->resultCode() ;

            _updateSubTask( curSubTask, newSubTask ) ;

            _updateOtherBySubTaskInfo() ;
         }
         else
         {
            PD_LOG( PDWARNING, "Sub-task[%u] in task[%llu] has already finished",
                    pSubTask->taskID(), _taskID ) ;
         }
      }
      else
      {
         PD_LOG( PDWARNING, "Sub-task[%u] doesn't exist in task[%llu]",
                 pSubTask->taskID(), _taskID ) ;
      }

      // to bson
      rc = _toChangedObj( NULL, &(itTask->second), matcher, updator ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build changed bson", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CLSIDXTASK_BADDGROUPBY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSIDXTASK_BRMGROUP, "_clsIdxTask::buildRemoveGroup" )
   INT32 _clsIdxTask::buildRemoveGroup( const CHAR* groupName,
                                        BSONObj& updator,
                                        BSONObj& matcher )
   {
      SDB_ASSERT( groupName, "group name can't be null" ) ;
      SDB_ASSERT( !_isMainTask, "can't be used by main-task" ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CLSIDXTASK_BRMGROUP ) ;
      MAP_GROUP_INFO_IT it ;

      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         PD_LOG( PDWARNING, "Task[%llu] has already finished", _taskID ) ;
         goto done ;
      }
      it = _mapGroupInfo.find( groupName ) ;
      if ( _mapGroupInfo.end() == it )
      {
         PD_LOG( PDWARNING, "Group[%s] doesn't exist in task[%llu]",
                 groupName, _taskID ) ;
         goto done ;
      }

      _clearChangedMask() ;

      // erase Groups, decrease SucceededGroups/FailedGroups/TotalGroups
      if ( CLS_TASK_STATUS_FINISH == it->second.status )
      {
         if ( _isSucceedGroup( it->second ) )
         {
            _decSucceededGroups() ;
         }
         else
         {
            _decFailedGroups() ;
         }
      }
      _decTotalGroups() ;

      _mapGroupInfo.erase( it ) ;
      _changedMask |= CLS_IDX_MASK_PULL_GROUP ;
      _pullGroupName = groupName ;

      // change other field
      _updateOtherByGroupInfo() ;

      // to bson
      rc = _toChangedObj( &(it->second), NULL, matcher, updator ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build changed bson", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CLSIDXTASK_BRMGROUP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSIDXTASK_BRMGROUPBY, "_clsIdxTask::buildRemoveGroupBy" )
   INT32 _clsIdxTask::buildRemoveGroupBy( const _clsTask* pSubTask,
                                          const ossPoolVector<BSONObj>& subTaskInfoList,
                                          const CHAR* groupName,
                                          BSONObj& updator,
                                          BSONObj& matcher )
   {
      SDB_ASSERT( pSubTask, "pSubTask cann't be null" ) ;
      SDB_ASSERT( groupName, "group name can't be null" ) ;
      SDB_ASSERT( _isMainTask, "should be used by main-task" ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CLSIDXTASK_BRMGROUPBY ) ;
      ossPoolMap<UINT64, clsSubTaskUnit>::iterator itTask ;
      MAP_GROUP_INFO_IT itGroup ;

      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         PD_LOG( PDWARNING, "Task[%llu] has already finished", _taskID ) ;
         goto done ;
      }

      _clearChangedMask() ;

      /// find out group unit from Groups List
      itGroup = _mapGroupInfo.find( groupName ) ;
      if ( itGroup != _mapGroupInfo.end() )
      {
         clsIdxTaskGroupUnit& curGroupInfo = itGroup->second ;
         if ( subTaskInfoList.empty() )
         {
            /// just remove this group
            if ( CLS_TASK_STATUS_FINISH == curGroupInfo.status )
            {
               if ( _isSucceedGroup( curGroupInfo ) )
               {
                  _decSucceededGroups() ;
               }
               else
               {
                  _decFailedGroups() ;
               }
            }
            _decTotalGroups() ;

            _changedMask |= CLS_IDX_MASK_PULL_GROUP ;
            _pullGroupName = curGroupInfo.groupName ;
            _mapGroupInfo.erase( itGroup ) ;
         }
         else
         {
            /// update this group info by sub-tasks
            clsIdxTaskGroupUnit newGroupInfo ;
            rc = _buildNewGroupInfo( subTaskInfoList, newGroupInfo ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to build new group info",
                         rc ) ;
            _updateGroup( curGroupInfo, newGroupInfo ) ;
         }
      }
      else
      {
         PD_LOG( PDWARNING, "Group[%s] doesn't exist in task[%llu]",
                 groupName, _taskID ) ;
      }

      /// find out sub-task in SubTasks list
      itTask = _mapSubTask.find( pSubTask->taskID() ) ;
      if ( itTask != _mapSubTask.end() )
      {
         clsSubTaskUnit& curSubTask = itTask->second ;
         if ( CLS_TASK_STATUS_FINISH != curSubTask.status )
         {
            /// generate new sub-task info
            clsSubTaskUnit newSubTask ;
            newSubTask.taskID     = pSubTask->taskID() ;
            newSubTask.taskType   = pSubTask->taskType() ;
            newSubTask.status     = pSubTask->status() ;
            newSubTask.resultCode = pSubTask->resultCode() ;

            /// change SubTasks / SucceedTasks / FailedTasks
            _updateSubTask( curSubTask, newSubTask ) ;

            /// change other fields
            _updateOtherBySubTaskInfo() ;
         }
         else
         {
            PD_LOG( PDWARNING, "Sub-task[%u] in task[%llu] has already finished",
                    pSubTask->taskID(), _taskID ) ;
         }
      }
      else
      {
         PD_LOG( PDWARNING, "Sub-task[%u] doesn't exist in task[%llu]",
                 pSubTask->taskID(), _taskID ) ;
      }

      rc = _toChangedObj( &(itGroup->second), &(itTask->second),
                          matcher, updator ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build changed bson", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CLSIDXTASK_BRMGROUPBY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsIdxTask::buildStartTask( const BSONObj& obj,
                                      BSONObj& updator,
                                      BSONObj& matcher )
   {
      SDB_ASSERT( !_isMainTask, "can't be used by main-task" ) ;
      return _buildStartTask( NULL, obj, updator, matcher ) ;
   }

   INT32 _clsIdxTask::buildStartTaskBy( const clsTask* pSubTask,
                                        const BSONObj& obj,
                                        BSONObj& updator,
                                        BSONObj& matcher )
   {
      SDB_ASSERT( pSubTask, "pSubTask cann't be null" ) ;
      SDB_ASSERT( _isMainTask, "should be used by main-task" ) ;
      return _buildStartTask( pSubTask, obj, updator, matcher ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSIDXTASK_BSTARTTASK, "_clsIdxTask::_buildStartTask" )
   INT32 _clsIdxTask::_buildStartTask( const clsTask* pSubTask,
                                       const BSONObj& obj,
                                       BSONObj& updator,
                                       BSONObj& matcher )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CLSIDXTASK_BSTARTTASK ) ;

      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         rc = SDB_TASK_ALREADY_FINISHED ;
         goto error ;
      }
      else if ( CLS_TASK_STATUS_CANCELED == _status )
      {
         rc = SDB_TASK_HAS_CANCELED ;
         goto error ;
      }
      else if ( CLS_TASK_STATUS_ROLLBACK == _status )
      {
         rc = SDB_TASK_ROLLBACK ;
         goto error ;
      }
      else if ( CLS_TASK_STATUS_READY == _status ||
                CLS_TASK_STATUS_RUN   == _status )
      {
         const CHAR* groupName = NULL ;
         clsIdxTaskGroupUnit* groupUnit = NULL ;
         clsSubTaskUnit* subTaskUnit = NULL ;

         // clear mask
         _clearChangedMask() ;

         // change task's status
         if ( CLS_TASK_STATUS_READY == _status )
         {
            setRun() ;
         }

         // change group's status
         rc = rtnGetStringElement( obj, FIELD_NAME_GROUPNAME, &groupName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field[%s], rc: %d",
                      FIELD_NAME_GROUPNAME, rc ) ;

         MAP_GROUP_INFO_IT it = _mapGroupInfo.find( groupName ) ;
         PD_CHECK( it != _mapGroupInfo.end(), SDB_SYS, error, PDERROR,
                   "Failed to find out group[%s] from task[%llu]",
                   groupName, _taskID ) ;

         groupUnit = &(it->second) ;

         if ( CLS_TASK_STATUS_READY == groupUnit->status )
         {
            groupUnit->status = CLS_TASK_STATUS_RUN ;
            _changedGroupMask |= CLS_IDX_MASK_STATUS ;
            _changedMask |= CLS_IDX_MASK_GROUPS ;
         }
         else if( CLS_TASK_STATUS_FINISH == groupUnit->status )
         {
            rc = SDB_TASK_ALREADY_FINISHED ;
            PD_LOG( PDINFO, "Task[%llu] on group[%s] has been finished, "
                    "do not start thread", _taskID, groupName ) ;
            goto done ;
         }

         // change sub-task's status
         if ( _isMainTask )
         {
            SDB_ASSERT( pSubTask, "pSubTask cann't be null" ) ;

            MAP_SUBTASK_IT itTask = _mapSubTask.find( pSubTask->taskID() ) ;
            PD_CHECK( itTask != _mapSubTask.end(), SDB_SYS, error, PDERROR,
                      "Failed to find out sub-task[%llu] from main task[%llu]",
                      pSubTask->taskID(), _taskID ) ;

            subTaskUnit = &(itTask->second) ;

            if ( CLS_TASK_STATUS_READY == subTaskUnit->status )
            {
               subTaskUnit->status = CLS_TASK_STATUS_RUN ;
               _changedSubtaskMask |= CLS_IDX_MASK_STATUS ;
               _changedMask |= CLS_IDX_MASK_SUBTASKS ;
            }
         }

         // to bson
         rc = _toChangedObj( groupUnit, subTaskUnit, matcher, updator ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build changed bson", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CLSIDXTASK_BSTARTTASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsIdxTask::_extractAndSetErrInfo( const BSONObj &errInfoArr,
                                             BSONObjBuilder &matcherBuilder,
                                             BSONObjBuilder &setBuilder )
   {
      INT32 rc = SDB_OK ;

      /* errInfoArr formate as below:
         [ { "GroupName": "db1", "ResultCode": -129, "Detail": "xxx" },
           ... ]
      */
      try
      {
         BSONObjIterator iter( errInfoArr ) ;
         while ( iter.more() )
         {
            clsIdxTaskGroupUnit* pGroup = NULL ;
            INT32 resultCode = SDB_OK ;
            const CHAR* groupName = NULL ;
            const CHAR* detail = NULL ;
            MAP_GROUP_INFO_IT it ;

            BSONElement ele = iter.next() ;
            PD_CHECK( ele.type() == Object, SDB_SYS, error,
                      PDERROR, "Invalid element type[%d]", ele.type() ) ;

            // get group name, result code, result detail
            BSONObj obj = ele.embeddedObject() ;
            rc = rtnGetStringElement( obj, FIELD_NAME_GROUPNAME, &groupName ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field[%s] from obj[%s], rc: %d",
                         FIELD_NAME_GROUPNAME, obj.toString().c_str(), rc ) ;

            // find the group from map
            it = _mapGroupInfo.find( groupName ) ;
            if ( it == _mapGroupInfo.end() )
            {
               continue ;
            }

            rc = rtnGetIntElement( obj, FIELD_NAME_RESULTCODE, resultCode ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field[%s] from obj[%s], rc: %d",
                         FIELD_NAME_RESULTCODE, obj.toString().c_str(), rc ) ;

            rc = rtnGetStringElement( obj, FIELD_NAME_DETAIL, &detail ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field[%s] from obj[%s], rc: %d",
                         FIELD_NAME_DETAIL, obj.toString().c_str(), rc ) ;

            pGroup = &(it->second) ;

            // clear mask
            _changedGroupMask = 0 ;

            // set result code
            if ( CLS_TASK_STATUS_READY == pGroup->status )
            {
               pGroup->resultCode = resultCode ;
               if ( detail && detail[0] != 0 )
               {
                  pGroup->resultInfo = BSON( FIELD_NAME_DETAIL << detail ) ;
               }
               _changedGroupMask |= CLS_IDX_MASK_RESULT ;
               _changedMask |= CLS_IDX_MASK_GROUPS ;
            }

            // to bson
            _toChangeGroupObj( matcherBuilder, setBuilder, *pGroup ) ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSIDXTASK_BCANCELTASK, "_clsIdxTask::buildCancelTask" )
   INT32 _clsIdxTask::buildCancelTask( const BSONObj& obj,
                                       BSONObj& updator,
                                       BSONObj& matcher )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CLSIDXTASK_BCANCELTASK ) ;
      INT32 resultCode = SDB_TASK_HAS_CANCELED ;
      BOOLEAN hasResultCode = TRUE ;
      BSONObj errInfoArr ;
      BSONObjBuilder matcherBuilder, setBuilder ;

      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         rc = SDB_TASK_ALREADY_FINISHED ;
         goto error ;
      }
      else if ( CLS_TASK_STATUS_CANCELED == _status )
      {
         rc = SDB_TASK_HAS_CANCELED ;
         goto error ;
      }
      else if ( CLS_TASK_STATUS_ROLLBACK == _status )
      {
         rc = SDB_TASK_ROLLBACK ;
         goto error ;
      }
      else if ( CLS_TASK_STATUS_RUN == _status )
      {
         if ( CLS_TASK_DROP_IDX == _taskType )
         {
            // When drop index is running, we can't cancel it.
            PD_LOG_MSG( PDERROR, "Cannot cancel drop index task[%llu] while "
                        "it is running", _taskID ) ;
            rc = SDB_TASK_CANNOT_CANCEL ;
            goto error ;
         }
      }

      // Internal cancellation, has ResultCode and ErrInfo, while user
      // cancellation doesn't has them.
      rc = rtnGetIntElement( obj, FIELD_NAME_RESULTCODE, resultCode ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         hasResultCode = FALSE ;
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the field[%s] from query",
                   FIELD_NAME_RESULTCODE ) ;

      rc = rtnGetArrayElement( obj, FIELD_NAME_ERROR_INFO, errInfoArr ) ;
      rc = SDB_FIELD_NOT_EXIST == rc ? SDB_OK : rc ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the field[%s] from query",
                   FIELD_NAME_ERROR_INFO ) ;

      /// clear mask
      _clearChangedMask() ;

      /// change Groups
      if ( !errInfoArr.isEmpty() )
      {
         rc = _extractAndSetErrInfo( errInfoArr, matcherBuilder, setBuilder ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      if ( CLS_TASK_STATUS_READY == _status )
      {
         /// change task's status
         setFinish( resultCode ) ;
         _toChangeOtherObj( matcherBuilder, setBuilder ) ;
      }
      else if ( CLS_TASK_STATUS_RUN == _status )
      {
         if ( !_isMainTask )
         {
            /// change task's status, result code
            setStatus( CLS_TASK_STATUS_CANCELED ) ;
            if ( hasResultCode )
            {
               // If ResultCode is not set here, task ResultCode will be -243
               // when task is finished. But it is an internal cancellation,
               // -243 ResultCode is not suitable.
               _resultCode = resultCode ;
               _changedMask |= CLS_IDX_MASK_RESULT ;
            }
         }
         else
         {
            /// change SubTasks status
            UINT32 cntFinish = 0 ;
            ossPoolMap<UINT64, clsSubTaskUnit>::iterator it ;
            for( it = _mapSubTask.begin() ; it != _mapSubTask.end() ; it++ )
            {
               clsSubTaskUnit &subTask = it->second ;
               if ( CLS_TASK_STATUS_FINISH == subTask.status )
               {
                  cntFinish++ ;
               }
               if ( CLS_TASK_STATUS_READY != subTask.status &&
                    CLS_TASK_STATUS_RUN != subTask.status )
               {
                  continue ;
               }

               // clear mask
               _changedSubtaskMask = 0 ;

               // change status, resultCode
               if ( CLS_TASK_STATUS_READY == subTask.status )
               {
                  subTask.status = CLS_TASK_STATUS_FINISH ;
                  subTask.resultCode = SDB_TASK_HAS_CANCELED ;
                  _changedSubtaskMask |= CLS_IDX_MASK_STATUS ;
                  _changedSubtaskMask |= CLS_IDX_MASK_RESULT ;
                  cntFinish++ ;
               }
               else if ( CLS_TASK_STATUS_RUN == subTask.status )
               {
                  subTask.status = CLS_TASK_STATUS_CANCELED ;
                  _changedSubtaskMask |= CLS_IDX_MASK_STATUS ;
               }
               _changedMask |= CLS_IDX_MASK_SUBTASKS ;

               // to bson
               _toChangeSubtaskObj( matcherBuilder, setBuilder, subTask ) ;
            }

            /// change task's status, result code
            if ( cntFinish == _mapSubTask.size() )
            {
               // Subtasks which already finished don't need to rolled back.
               setFinish( resultCode ) ;
            }
            else
            {
               setStatus( CLS_TASK_STATUS_CANCELED ) ;
               if ( hasResultCode )
               {
                  _resultCode = resultCode ;
                  _changedMask |= CLS_IDX_MASK_RESULT ;
               }
            }
         }

         _toChangeOtherObj( matcherBuilder, setBuilder ) ;
      }

      /// to bson
      matcher = matcherBuilder.obj() ;
      updator = BSON( "$set" << setBuilder.obj() ) ;

   done:
      PD_TRACE_EXITRC( SDB_CLSIDXTASK_BCANCELTASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSIDXTASK_BCANCELTASKBY, "_clsIdxTask::buildCancelTaskBy" )
   INT32 _clsIdxTask::buildCancelTaskBy( const clsTask* pSubTask,
                                         BSONObj& updator,
                                         BSONObj& matcher )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CLSIDXTASK_BCANCELTASKBY ) ;
      clsSubTaskUnit* subTaskUnit = NULL ;

      SDB_ASSERT( pSubTask, "pSubTask cann't be null" ) ;
      SDB_ASSERT( _isMainTask, "should be used by main-task" ) ;

      // clear mask
      _clearChangedMask() ;

      // change SubTasks status and result code
      MAP_SUBTASK_IT itTask = _mapSubTask.find( pSubTask->taskID() ) ;
      PD_CHECK( itTask != _mapSubTask.end(), SDB_SYS, error, PDERROR,
                "Failed to find out sub-task[%llu] from main task[%llu]",
                pSubTask->taskID(), _taskID ) ;

      subTaskUnit = &(itTask->second) ;

      if ( subTaskUnit->status != pSubTask->status() )
      {
         subTaskUnit->status = pSubTask->status() ;
         _changedSubtaskMask |= CLS_IDX_MASK_STATUS ;
      }
      if ( subTaskUnit->resultCode != pSubTask->resultCode() )
      {
         subTaskUnit->resultCode = pSubTask->resultCode() ;
         _changedSubtaskMask |= CLS_IDX_MASK_RESULT ;
      }

      // change task status
      _updateOtherBySubTaskInfo() ;

      // to bson
      _changedMask |= CLS_IDX_MASK_SUBTASKS ;

      rc = _toChangedObj( NULL, subTaskUnit, matcher, updator ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build changed bson", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CLSIDXTASK_BCANCELTASKBY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSIDXTASK_BRPTTASK, "_clsIdxTask::buildReportTask" )
   INT32 _clsIdxTask::buildReportTask( const BSONObj& obj,
                                       BSONObj& updator,
                                       BSONObj& matcher )
   {
      SDB_ASSERT( !_isMainTask, "cann't be used by main-task" ) ;
      PD_TRACE_ENTRY( SDB_CLSIDXTASK_BRPTTASK ) ;

      INT32 rc = SDB_OK ;
      clsIdxTaskGroupUnit newGroupInfo ;
      MAP_GROUP_INFO_IT it ;

      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         PD_LOG( PDWARNING, "Task[%llu] has already finished", _taskID ) ;
         goto done ;
      }
      if ( _status != CLS_TASK_STATUS_RUN &&
           _status != CLS_TASK_STATUS_CANCELED &&
           _status != CLS_TASK_STATUS_ROLLBACK )
      {
         rc = SDB_CAT_TASK_STATUS_ERROR ;
         PD_LOG( PDERROR, "Task[%llu] status[%s] error",
                 _taskID, clsTaskStatusStr(_status) ) ;
         goto error ;
      }

      /// init group unit
      rc = newGroupInfo.init( obj.objdata() ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to init group info, rc: %d",
                   rc ) ;

      /// find out group unit from Groups List
      it = _mapGroupInfo.find( newGroupInfo.groupName ) ;
      PD_CHECK( it != _mapGroupInfo.end(), SDB_SYS, error, PDERROR,
                "Failed to find out group[%s] from task[%llu]",
                newGroupInfo.groupName.c_str(), _taskID ) ;

      if ( CLS_TASK_STATUS_READY == it->second.status &&
           CLS_TASK_STATUS_FINISH == newGroupInfo.status )
      {
         rc = SDB_CAT_TASK_STATUS_ERROR ;
         PD_LOG( PDWARNING, "Group[%s] status[%s] error in task[%llu], rc: %d",
                 newGroupInfo.groupName.c_str(), VALUE_NAME_READY, _taskID, rc ) ;
         goto error ;
      }

      if ( CLS_TASK_STATUS_FINISH == it->second.status &&
           ( newGroupInfo.resultCode == it->second.resultCode ||
             newGroupInfo.resultCode == SDB_TASK_ALREADY_FINISHED  ) )
      {
         // 'finish + ok' can convert to 'finish + -243'
         goto done ;
      }

      /// update info
      _clearChangedMask() ;

      _updateGroup( it->second, newGroupInfo ) ;

      _updateOtherByGroupInfo() ;

      rc = _toChangedObj( &(it->second), NULL, matcher, updator ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build changed bson", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CLSIDXTASK_BRPTTASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSIDXTASK_BRPTTASKBY, "_clsIdxTask::buildReportTaskBy" )
   INT32 _clsIdxTask::buildReportTaskBy( const clsTask* pSubTask,
                                         const ossPoolVector<BSONObj>& subTaskInfoList,
                                         BSONObj& updator,
                                         BSONObj& matcher )
   {
      SDB_ASSERT( pSubTask, "pSubTask cann't be null" ) ;
      SDB_ASSERT( _isMainTask, "should be used by main-task" ) ;
      PD_TRACE_ENTRY( SDB_CLSIDXTASK_BRPTTASKBY ) ;

      INT32 rc = SDB_OK ;
      BOOLEAN needChangeGroup = TRUE ;
      BOOLEAN needChangeSubtask = TRUE ;
      const clsIdxTask* pTask = NULL ;
      ossPoolMap<UINT64, clsSubTaskUnit>::iterator itTask ;
      MAP_GROUP_INFO_IT itGroup ;
      clsIdxTaskGroupUnit newGroupInfo ;

      if ( CLS_TASK_STATUS_FINISH == _status )
      {
         PD_LOG( PDWARNING, "Task[%llu] has already finished", _taskID ) ;
         goto done ;
      }

      pTask = dynamic_cast<const clsIdxTask*>( pSubTask ) ;
      PD_CHECK( NULL != pTask, SDB_INVALIDARG, error, PDERROR,
                "Failed to get index task" ) ;

      /// find out sub-task in SubTasks list
      itTask = _mapSubTask.find( pTask->taskID() ) ;
      PD_CHECK( itTask != _mapSubTask.end(), SDB_SYS, error, PDERROR,
                "Failed to find out sub-task[%llu] from main task[%llu]",
                pTask->taskID(), _taskID ) ;

      if ( CLS_TASK_STATUS_FINISH == itTask->second.status )
      {
         needChangeSubtask = FALSE ;
         PD_LOG( PDWARNING, "Sub-task[%llu] of task[%llu] has already finished",
                 pTask->taskID(), _taskID ) ;
      }

      /// generate new group info by sub-tasks
      rc = _buildNewGroupInfo( subTaskInfoList, newGroupInfo ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build new group info",
                   rc ) ;

      /// find out group unit from Groups List
      itGroup = _mapGroupInfo.find( newGroupInfo.groupName ) ;
      PD_CHECK( itGroup != _mapGroupInfo.end(), SDB_SYS, error, PDERROR,
                "Failed to find out group[%s] from task[%llu]",
                newGroupInfo.groupName.c_str(), _taskID ) ;

      if ( CLS_TASK_STATUS_FINISH == itGroup->second.status &&
           newGroupInfo.resultCode == itGroup->second.resultCode )
      {
         // 'finish + ok' can convert to 'finish + -243'
         needChangeGroup = FALSE ;
         PD_LOG( PDWARNING, "Group[%s] in task[%llu] has already finished",
                 newGroupInfo.groupName.c_str(), _taskID ) ;
      }

      /// update info
      if ( needChangeGroup || needChangeSubtask )
      {
         _clearChangedMask() ;

         if ( needChangeGroup )
         {
            _updateGroup( itGroup->second, newGroupInfo ) ;
         }

         if ( needChangeSubtask )
         {
            /// generate new sub-task info
            clsSubTaskUnit newSubTask ;
            newSubTask.taskID     = pTask->taskID() ;
            newSubTask.taskType   = pTask->taskType() ;
            newSubTask.status     = pTask->status() ;
            newSubTask.resultCode = pTask->resultCode() ;

            _updateSubTask( itTask->second, newSubTask ) ;

            _updateOtherBySubTaskInfo() ;
         }

         rc = _toChangedObj( &(itGroup->second), &(itTask->second),
                             matcher, updator ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build changed bson", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CLSIDXTASK_BRPTTASKBY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _clsIdxTask::_updateSubTask( clsSubTaskUnit& curSubTask,
                                     const clsSubTaskUnit& newSubTask )
   {
      /*
      * Update SubTasks / SucceededTasks / FailedTasks field
      */

      BOOLEAN hasChangeSubtask = FALSE ;
      BOOLEAN change2Finish = FALSE ; // run -> finish

      if ( CLS_TASK_STATUS_FINISH == curSubTask.status )
      {
         goto done ;
      }

      /// update SubTasks element
      if ( curSubTask.status != newSubTask.status )
      {
         curSubTask.status = newSubTask.status ;
         _changedSubtaskMask |= CLS_IDX_MASK_STATUS ;
         hasChangeSubtask = TRUE ;
         if ( CLS_TASK_STATUS_FINISH == newSubTask.status )
         {
            change2Finish = TRUE ;
         }
      }
      if ( change2Finish || curSubTask.resultCode != newSubTask.resultCode )
      {
         curSubTask.resultCode = newSubTask.resultCode ;
         _changedSubtaskMask |= CLS_IDX_MASK_RESULT ;
         hasChangeSubtask = TRUE ;
      }

      if ( hasChangeSubtask )
      {
         _changedMask |= CLS_IDX_MASK_SUBTASKS ;
      }

      // increase sub-task count
      if ( CLS_TASK_STATUS_FINISH == newSubTask.status )
      {
         if ( _isSucceedTask( newSubTask ) )
         {
            _incSucceededTasks() ;
         }
         else
         {
            _incFailedTasks() ;
         }
      }

   done:
      return ;
   }


   BOOLEAN _clsIdxTask::_isSucceedGroup( const clsIdxTaskGroupUnit& groupInfo )
   {
      return _isSucceed( _taskType, groupInfo.status, groupInfo.resultCode ) ;
   }

   BOOLEAN _clsIdxTask::_isSucceedTask( const clsSubTaskUnit& subTaskInfo )
   {
      return _isSucceed( subTaskInfo.taskType, subTaskInfo.status,
                         subTaskInfo.resultCode ) ;
   }

   BOOLEAN _clsIdxTask::_isSucceed( CLS_TASK_TYPE taskType,
                                    CLS_TASK_STATUS status,
                                    INT32 resultCode )
   {
      BOOLEAN isSucc = FALSE ;

      SDB_ASSERT( CLS_TASK_STATUS_FINISH == status,
                  "Group status must be finished") ;

      if ( SDB_OK == resultCode )
      {
         isSucc = TRUE ;
      }
      else if ( SDB_IXM_REDEF == resultCode )
      {
         if ( CLS_TASK_CREATE_IDX == taskType ||
              CLS_TASK_COPY_IDX   == taskType )
         {
            isSucc = TRUE ;
         }
      }
      else if ( SDB_IXM_NOTEXIST == resultCode )
      {
         if ( CLS_TASK_DROP_IDX == taskType )
         {
            isSucc = TRUE ;
         }
      }

      return isSucc ;
   }

   void _clsIdxTask::_incSucceededGroups()
   {
      _succeededGroups++ ;
      _changedMask |= CLS_IDX_MASK_GROUPCOUNT ;
   }

   void _clsIdxTask::_decSucceededGroups()
   {
      _succeededGroups-- ;
      _changedMask |= CLS_IDX_MASK_GROUPCOUNT ;
   }

   void _clsIdxTask::_incFailedGroups()
   {
      _failedGroups++ ;
      _changedMask |= CLS_IDX_MASK_GROUPCOUNT ;
   }

   void _clsIdxTask::_decFailedGroups()
   {
      _failedGroups-- ;
      _changedMask |= CLS_IDX_MASK_GROUPCOUNT ;
   }

   void _clsIdxTask::_incTotalGroups()
   {
      _totalGroups++ ;
      _changedMask |= CLS_IDX_MASK_GROUPCOUNT ;
   }

   void _clsIdxTask::_decTotalGroups()
   {
      _totalGroups-- ;
      _changedMask |= CLS_IDX_MASK_GROUPCOUNT ;
   }

   void _clsIdxTask::_incSucceededTasks()
   {
      _succeededTasks++ ;
      _changedMask |= CLS_IDX_MASK_TASKCOUNT ;
   }

   void _clsIdxTask::_decSucceededTasks()
   {
      _succeededTasks-- ;
      _changedMask |= CLS_IDX_MASK_TASKCOUNT ;
   }

   void _clsIdxTask::_incFailedTasks()
   {
      _failedTasks++ ;
      _changedMask |= CLS_IDX_MASK_TASKCOUNT ;
   }

   void _clsIdxTask::_decFailedTasks()
   {
      _failedTasks-- ;
      _changedMask |= CLS_IDX_MASK_TASKCOUNT ;
   }

   void _clsIdxTask::_incTotalTasks()
   {
      _totalTasks++ ;
      _changedMask |= CLS_IDX_MASK_TASKCOUNT ;
   }

   void _clsIdxTask::_decTotalTasks()
   {
      _totalTasks-- ;
      _changedMask |= CLS_IDX_MASK_TASKCOUNT ;
   }

   void _clsIdxTask::_updateGroup( clsIdxTaskGroupUnit& curGroupInfo,
                                   const clsIdxTaskGroupUnit& newGroupInfo )
   {
      /*
      * Update Groups / SucceededGroups / FailedGroups field
      */

      BOOLEAN hasChangedGroup = FALSE ;
      BOOLEAN change2Finish = FALSE ; // run -> finish

      /// increase group count
      if ( CLS_TASK_STATUS_FINISH == curGroupInfo.status &&
           CLS_TASK_STATUS_FINISH == newGroupInfo.status )
      {
         // 'finish + ok' can convert to 'finish + -243',
         // we should DECREASE _succeededGroups
         if (  _isSucceedGroup( curGroupInfo ) &&
              !_isSucceedGroup( newGroupInfo ) )
         {
            _decSucceededGroups() ;
            _incFailedGroups() ;
         }
         else if ( !_isSucceedGroup( curGroupInfo ) &&
                    _isSucceedGroup( newGroupInfo ) )
         {
            _decFailedGroups() ;
            _incSucceededGroups() ;
         }
      }
      else if ( CLS_TASK_STATUS_FINISH == newGroupInfo.status )
      {
         if ( _isSucceedGroup( newGroupInfo ) )
         {
            _incSucceededGroups() ;
         }
         else
         {
            _incFailedGroups() ;
         }
      }

      /// update Groups element
      if ( curGroupInfo.status != newGroupInfo.status )
      {
         curGroupInfo.status = newGroupInfo.status ;
         _changedGroupMask |= CLS_IDX_MASK_STATUS ;
         hasChangedGroup = TRUE ;
         if ( CLS_TASK_STATUS_FINISH == newGroupInfo.status )
         {
            change2Finish = TRUE ;
         }
      }
      if ( change2Finish || curGroupInfo.resultCode != newGroupInfo.resultCode )
      {
         curGroupInfo.resultCode = newGroupInfo.resultCode ;
         curGroupInfo.resultInfo= newGroupInfo.resultInfo ;
         _changedGroupMask |= CLS_IDX_MASK_RESULT ;
         hasChangedGroup = TRUE ;
      }
      if ( curGroupInfo.opInfo != newGroupInfo.opInfo )
      {
         curGroupInfo.opInfo = newGroupInfo.opInfo;
         _changedGroupMask |= CLS_IDX_MASK_OPINFO ;
         hasChangedGroup = TRUE ;
      }
      if ( curGroupInfo.retryCnt != newGroupInfo.retryCnt )
      {
         curGroupInfo.retryCnt = newGroupInfo.retryCnt ;
         _changedGroupMask |= CLS_IDX_MASK_RETRYCNT ;
         hasChangedGroup = TRUE ;
      }
      if ( curGroupInfo.timeSpent != newGroupInfo.timeSpent )
      {
         curGroupInfo.progress = newGroupInfo.progress ;
         curGroupInfo.speed = newGroupInfo.speed ;
         curGroupInfo.timeSpent = newGroupInfo.timeSpent ;
         curGroupInfo.timeLeft = newGroupInfo.timeLeft ;
         _changedGroupMask |= CLS_IDX_MASK_PROGRESS ;
         hasChangedGroup = TRUE ;
      }
      if ( curGroupInfo.totalRecNum != newGroupInfo.totalRecNum )
      {
         curGroupInfo.totalRecNum = newGroupInfo.totalRecNum ;
         _changedGroupMask |= CLS_IDX_MASK_TOTALREC ;
         hasChangedGroup = TRUE ;
      }
      if ( curGroupInfo.pcsedRecNum != newGroupInfo.pcsedRecNum )
      {
         curGroupInfo.pcsedRecNum = newGroupInfo.pcsedRecNum ;
         _changedGroupMask |= CLS_IDX_MASK_PCSEDREC ;
         hasChangedGroup = TRUE ;
      }

      if ( hasChangedGroup )
      {
         _changedMask |= CLS_IDX_MASK_GROUPS ;
      }
   }

   INT32 _clsIdxTask::_buildNewGroupInfo( const ossPoolVector<BSONObj> &subTaskInfoList,
                                          clsIdxTaskGroupUnit &newGroupInfo )
   {
      INT32 rc = SDB_OK ;
      INT32 totalCnt = subTaskInfoList.size() ;
      INT32 finishCnt = 0 ;
      INT32 readyCnt = 0 ;
      INT32 firstResultCode = SDB_OK ;
      BSONObj firstResultInfo ;

      for( ossPoolVector<BSONObj>::const_iterator it = subTaskInfoList.begin() ;
           it != subTaskInfoList.end() ; ++it )
      {
         // two format:
         // 1: { "Groups": [ { "GroupName": "db1", Status: xxx, ... } ] }
         // 2:               { "GroupName": "db1", Status: xxx, ... }
         const BSONObj &subTaskInfo = *it ;
         BSONObj groupObj ;
         BSONElement ele ;

         ele = subTaskInfo.firstElement() ;
         if ( Array == ele.type() )
         {
            BSONObj array = ele.Obj() ;
            ele = array.firstElement() ;
            PD_CHECK( ele.type() == Object,
                      SDB_INVALIDARG, error, PDERROR,
                      "Invalid first element type[%d] in obj[%s]",
                      ele.type(), array.toString().c_str() ) ;

            groupObj = ele.Obj() ;
         }
         else
         {
            groupObj = subTaskInfo ;
         }

         // init current group unit
         clsIdxTaskGroupUnit groupUnit ;
         rc = groupUnit.init( groupObj.objdata() ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to init group unit, rc: %d",
                      rc ) ;

         // check GroupName
         if ( it == subTaskInfoList.begin() )
         {
            newGroupInfo.groupName = groupUnit.groupName ;
         }
         else
         {
            PD_CHECK( newGroupInfo.groupName == groupUnit.groupName,
                      SDB_INVALIDARG, error, PDERROR,
                      "Invalid group name[%s], expect[%s]",
                      groupUnit.groupName.c_str(), newGroupInfo.groupName.c_str() ) ;
         }

         // count group
         if ( CLS_TASK_STATUS_FINISH == groupUnit.status )
         {
            finishCnt++ ;
         }
         else if ( CLS_TASK_STATUS_READY == groupUnit.status )
         {
            readyCnt++ ;
         }
         if ( SDB_OK == firstResultCode &&
              SDB_OK != groupUnit.resultCode )
         {
            firstResultCode = groupUnit.resultCode ;
            firstResultInfo = groupUnit.resultInfo ;
         }

         // TotalSize ProcessedSize RetryCnt Speed
         newGroupInfo.totalRecNum += groupUnit.totalRecNum ;
         newGroupInfo.pcsedRecNum += groupUnit.pcsedRecNum ;
         newGroupInfo.retryCnt    += groupUnit.retryCnt ;
         newGroupInfo.speed       += groupUnit.speed ;

         // TimeSpent TimeLeft
         if ( groupUnit.timeSpent > newGroupInfo.timeSpent )
         {
            newGroupInfo.timeSpent = groupUnit.timeSpent ;
         }
         if ( groupUnit.timeLeft > newGroupInfo.timeLeft )
         {
            newGroupInfo.timeLeft = groupUnit.timeLeft ;
         }
      }

      // Status
      if ( finishCnt == totalCnt )
      {
         newGroupInfo.status = CLS_TASK_STATUS_FINISH ;
      }
      else if ( readyCnt == totalCnt )
      {
         newGroupInfo.status = CLS_TASK_STATUS_READY ;
      }
      else
      {
         newGroupInfo.status = CLS_TASK_STATUS_RUN ;
      }

      // ResultCode ResultInfo
      if ( CLS_TASK_STATUS_FINISH == newGroupInfo.status )
      {
         newGroupInfo.resultCode = firstResultCode ;
         newGroupInfo.resultInfo = firstResultInfo ;
      }

      // Progress
      newGroupInfo.progress = (FLOAT64)newGroupInfo.pcsedRecNum /
                                       newGroupInfo.totalRecNum * 100 ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _clsIdxTask::_updateOtherByGroupInfo()
   {
      /*
      * Update other fields except Groups / SucceedGroups / FailedGroups
      */
      SDB_ASSERT( !_isMainTask, "cann't be used by main-task" ) ;
      UINT32 cntTotalGroup = _mapGroupInfo.size() ;
      UINT32 cntReadyGroup = 0 ;
      UINT32 cntSucGroup = 0 ;
      UINT32 cntFailGroup = 0 ;
      UINT32 cntRedefineIdx = 0 ;
      UINT32 cntNotExistIdx = 0 ;
      UINT32 cntIgnoreError = 0 ;
      INT32 firstResultCode = SDB_OK ;
      BSONObj firstResultInfo ;

      // loop every group
      for( MAP_GROUP_INFO_IT it = _mapGroupInfo.begin() ;
           it != _mapGroupInfo.end() ;
           it++ )
      {
         clsIdxTaskGroupUnit &localGroup = it->second ;
         if ( CLS_TASK_STATUS_READY == localGroup.status )
         {
            cntReadyGroup++ ;
         }
         else if ( CLS_TASK_STATUS_FINISH == localGroup.status )
         {
            if ( SDB_OK == localGroup.resultCode )
            {
               cntSucGroup++ ;
            }
            else
            {
               cntFailGroup++ ;
               if ( SDB_IXM_REDEF == localGroup.resultCode &&
                    CLS_TASK_CREATE_IDX == _taskType )
               {
                  cntRedefineIdx++ ;
               }
               else if ( SDB_IXM_NOTEXIST == localGroup.resultCode &&
                         CLS_TASK_DROP_IDX == _taskType )
               {
                  cntNotExistIdx++ ;
               }
               else if ( SDB_DMS_CS_NOTEXIST == localGroup.resultCode ||
                         SDB_DMS_NOTEXIST    == localGroup.resultCode )
               {
                  cntIgnoreError++ ;
               }
               else
               {
                  // When one group failed, other groups will be rolled back
                  // ( error code is SDB_TASK_ROLLBACK ). So we looks up other
                  // error code besides SDB_TASK_ROLLBACK.
                  if ( SDB_OK == firstResultCode ||
                       SDB_TASK_ROLLBACK == firstResultCode )
                  {
                     firstResultCode = localGroup.resultCode ;
                     firstResultInfo = localGroup.resultInfo ;
                  }
               }
            }
         }
      }

      cntIgnoreError += ( cntRedefineIdx + cntNotExistIdx ) ;

      // set first resultCode
      if ( cntTotalGroup != 0 )
      {
         if ( CLS_TASK_CREATE_IDX == _taskType &&
              cntRedefineIdx == cntTotalGroup )
         {
            // if all groups return -247 error, DO NOT ignore error
            firstResultCode = SDB_IXM_REDEF ;
         }
         else if ( CLS_TASK_DROP_IDX == _taskType &&
                   cntNotExistIdx == cntTotalGroup )
         {
            // if all groups return -47 error, DO NOT ignore error
            firstResultCode = SDB_IXM_NOTEXIST ;
         }
      }

      // switch status
      if ( 0 == cntTotalGroup )
      {
         setFinish() ;
      }
      else if ( CLS_TASK_STATUS_READY == _status )
      {
         if ( cntReadyGroup == cntTotalGroup )
         {
            // If buildAddGroup() add another Ready group,
            // just keep Ready status
         }
         else
         {
            setRun() ;
         }
      }
      else if ( CLS_TASK_STATUS_RUN == _status )
      {
         if ( cntRedefineIdx == cntTotalGroup )
         {
            // all groups report error -247
            setFinish( SDB_IXM_REDEF ) ;
         }
         else if ( cntNotExistIdx == cntTotalGroup )
         {
            // all groups report error -47
            setFinish( SDB_IXM_NOTEXIST ) ;
         }
         else if ( cntSucGroup + cntIgnoreError == cntTotalGroup )
         {
            // all groups succeed( may include -247/-47/-23/-34 )
            setFinish() ;
         }
         else if ( CLS_TASK_DROP_IDX == _taskType )
         {
            // When drop index is running, we can't roll back it.
            if ( cntSucGroup + cntFailGroup == cntTotalGroup )
            {
               // all groups finish
               setFinish( firstResultCode, firstResultInfo ) ;
            }
         }
         else
         {
            if ( cntFailGroup - cntIgnoreError > 0 )
            {
               // some groups failed
               if ( cntReadyGroup + cntFailGroup == cntTotalGroup )
               {
                  // none of groups did it, so just finish task
                  setFinish( firstResultCode, firstResultInfo ) ;
               }
               else
               {
                  // some groups failed, we need rollback other groups
                  setStatus( CLS_TASK_STATUS_ROLLBACK ) ;
               }
            }
         }
      }
      else if ( CLS_TASK_STATUS_ROLLBACK == _status )
      {
         if ( cntReadyGroup + cntFailGroup == cntTotalGroup )
         {
            // none of groups did it, so just finish task
            setFinish( firstResultCode, firstResultInfo ) ;
         }
      }
      else if ( CLS_TASK_STATUS_CANCELED == _status )
      {
         if ( cntReadyGroup + cntFailGroup == cntTotalGroup )
         {
            // none of groups did it, so just finish task
            setFinish( SDB_TASK_HAS_CANCELED ) ;
         }
      }

      // caculate progress totally
      if ( CLS_TASK_STATUS_RUN    == _status ||
           CLS_TASK_STATUS_FINISH == _status )
      {
         _calculate() ;
      }
   }

   void _clsIdxTask::_updateOtherBySubTaskInfo()
   {
      /*
      * Update other fields except Groups / SucceedGroups / FailedGroups /
      *                            SubTasks / SucceedTasks / FailedTasks
      */
      SDB_ASSERT( _isMainTask, "should be used by main-task" ) ;
      UINT32 cntFinished = 0 ;
      UINT32 cntTotal = _mapSubTask.size() ;
      UINT32 cntRedefineIdx = 0 ;
      UINT32 cntNotExistIdx = 0 ;
      INT32 firstResultCode = SDB_OK ;

      // loop every subTask
      for( ossPoolMap<UINT64, clsSubTaskUnit>::iterator it = _mapSubTask.begin() ;
           it != _mapSubTask.end() ; it++ )
      {
         clsSubTaskUnit &localSubTask = it->second ;
         if ( CLS_TASK_STATUS_FINISH == localSubTask.status )
         {
            cntFinished++ ;
            if ( SDB_IXM_REDEF == localSubTask.resultCode &&
                 CLS_TASK_CREATE_IDX == localSubTask.taskType )
            {
               cntRedefineIdx++ ;
            }
            else if ( SDB_IXM_NOTEXIST == localSubTask.resultCode &&
                      CLS_TASK_DROP_IDX == localSubTask.taskType )
            {
               cntNotExistIdx++ ;
            }
            else
            {
               if ( SDB_OK == firstResultCode )
               {
                  firstResultCode = localSubTask.resultCode ;
               }
            }
         }
      }

      // set first resultCode
      if ( cntTotal != 0 )
      {
         if ( cntRedefineIdx == cntTotal )
         {
            firstResultCode = SDB_IXM_REDEF ;
         }
         else if ( cntNotExistIdx == cntTotal )
         {
            firstResultCode = SDB_IXM_NOTEXIST ;
         }
      }

      // switch status
      if ( 0 == cntTotal )
      {
         setFinish() ;
      }
      else if ( CLS_TASK_STATUS_READY == _status )
      {
         setRun() ;
      }
      else if ( CLS_TASK_STATUS_RUN == _status )
      {
         if ( cntFinished == cntTotal )
         {
            setFinish( firstResultCode ) ;
         }
      }
      else if ( CLS_TASK_STATUS_CANCELED == _status )
      {
         if ( cntFinished == cntTotal )
         {
            setFinish( SDB_TASK_HAS_CANCELED ) ;
         }
      }

      // caculate progress totally
      if ( CLS_TASK_STATUS_RUN    == _status ||
           CLS_TASK_STATUS_FINISH == _status )
      {
         _calculate() ;
      }
   }

   void _clsIdxTask::_clearChangedMask()
   {
      _changedMask = 0 ;
      _changedGroupMask = 0 ;
      _changedSubtaskMask = 0 ;
      _i = 1 ;
      _pullSubTaskID = CLS_INVALID_TASKID ;
      _pullGroupName.clear() ;
      _pushGroupName.clear() ;
   }

   void _clsIdxTask::_toChangeOtherObj( BSONObjBuilder& matcherB,
                                        BSONObjBuilder& setB )
   {
      matcherB.append( FIELD_NAME_TASKID, (INT64)_taskID ) ;

      if ( _changedMask & CLS_IDX_MASK_STATUS )
      {
         setB.append( FIELD_NAME_STATUS, _status ) ;
         setB.append( FIELD_NAME_STATUSDESC, clsTaskStatusStr(_status) ) ;
      }
      if ( _changedMask & CLS_IDX_MASK_RESULT )
      {
         setB.append( FIELD_NAME_RESULTCODE, _resultCode ) ;
         setB.append( FIELD_NAME_RESULTCODEDESC, getErrDesp(_resultCode) ) ;
         setB.append( FIELD_NAME_RESULTINFO, _resultInfo ) ;
      }
      if ( _changedMask & CLS_IDX_MASK_PROGRESS )
      {
         setB.append( FIELD_NAME_PROGRESS, _progress ) ;
         setB.append( FIELD_NAME_SPEED, (INT64)_speed ) ;
         setB.append( FIELD_NAME_TIMESPENT, _timeSpent ) ;
         setB.append( FIELD_NAME_TIMELEFT, _timeLeft ) ;
      }
      if ( _changedMask & CLS_IDX_MASK_GROUPCOUNT )
      {
         setB.append( FIELD_NAME_TOTALGROUP, _totalGroups ) ;
         setB.append( FIELD_NAME_SUCCEEDGROUP, _succeededGroups ) ;
         setB.append( FIELD_NAME_FAILGROUP, _failedGroups ) ;
      }
      if ( _changedMask & CLS_IDX_MASK_TASKCOUNT )
      {
         setB.append( FIELD_NAME_TOTALSUBTASK, _totalTasks ) ;
         setB.append( FIELD_NAME_SUCCEEDSUBTASK, _succeededTasks ) ;
         setB.append( FIELD_NAME_FAILSUBTASK, _failedTasks ) ;
      }
      if ( _changedMask & CLS_IDX_MASK_BEGINTIME )
      {
         CHAR timeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossTimestampToString( _beginTS, timeStr ) ;
         setB.append( FIELD_NAME_BEGINTIMESTAMP, timeStr ) ;
      }
      if ( _changedMask & CLS_IDX_MASK_ENDTIME )
      {
         CHAR timeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossTimestampToString( _endTS, timeStr ) ;
         setB.append( FIELD_NAME_ENDTIMESTAMP, timeStr ) ;
      }
   }

   void _clsIdxTask::_toChangeSubtaskObj( BSONObjBuilder& matcherB,
                                          BSONObjBuilder& setB,
                                          const clsSubTaskUnit& subTask )
   {
      if ( _changedMask & CLS_IDX_MASK_SUBTASKS )
      {
         CHAR tmp[64] = { 0 } ;
         _i++ ;

         ossSnprintf( tmp, 64, "%s.$%u.%s",
                      FIELD_NAME_SUBTASKS, _i, FIELD_NAME_TASKID ) ;
         matcherB.append( tmp, (INT64)subTask.taskID ) ;

         if ( _changedSubtaskMask & CLS_IDX_MASK_STATUS )
         {
            ossSnprintf( tmp, 64, "%s.$%u.%s",
                         FIELD_NAME_SUBTASKS, _i, FIELD_NAME_STATUS ) ;
            setB.append( tmp, subTask.status ) ;
            ossSnprintf( tmp, 64, "%s.$%u.%s",
                         FIELD_NAME_SUBTASKS, _i, FIELD_NAME_STATUSDESC ) ;
            setB.append( tmp, clsTaskStatusStr( subTask.status ) ) ;
         }
         if ( _changedSubtaskMask & CLS_IDX_MASK_RESULT )
         {
            ossSnprintf( tmp, 64, "%s.$%u.%s",
                         FIELD_NAME_SUBTASKS, _i, FIELD_NAME_RESULTCODE ) ;
            setB.append( tmp, subTask.resultCode ) ;
         }
      }
   }

   void _clsIdxTask::_toChangeGroupObj( BSONObjBuilder& matcherB,
                                        BSONObjBuilder& setB,
                                        const clsIdxTaskGroupUnit& group )
   {
      if ( _changedMask & CLS_IDX_MASK_GROUPS )
      {
         CHAR tmp[64] = { 0 } ;
         _i++ ;

         ossSnprintf( tmp, 64, "%s.$%u.%s",
                      FIELD_NAME_GROUPS, _i, FIELD_NAME_GROUPNAME ) ;
         matcherB.append( tmp, group.groupName.c_str() ) ;

         if ( _changedGroupMask & CLS_IDX_MASK_STATUS )
         {
            ossSnprintf( tmp, 64, "%s.$%u.%s",
                         FIELD_NAME_GROUPS, _i, FIELD_NAME_STATUS ) ;
            setB.append( tmp, group.status ) ;

            ossSnprintf( tmp, 64, "%s.$%u.%s",
                         FIELD_NAME_GROUPS, _i, FIELD_NAME_STATUSDESC ) ;
            setB.append( tmp, clsTaskStatusStr( group.status ) ) ;
         }
         if ( _changedGroupMask & CLS_IDX_MASK_RESULT )
         {
            ossSnprintf( tmp, 64, "%s.$%u.%s",
                         FIELD_NAME_GROUPS, _i, FIELD_NAME_RESULTCODE ) ;
            setB.append( tmp, group.resultCode ) ;

            ossSnprintf( tmp, 64, "%s.$%u.%s",
                         FIELD_NAME_GROUPS, _i, FIELD_NAME_RESULTCODEDESC ) ;
            setB.append( tmp, getErrDesp( group.resultCode ) ) ;

            ossSnprintf( tmp, 64, "%s.$%u.%s",
                         FIELD_NAME_GROUPS, _i, FIELD_NAME_RESULTINFO ) ;
            setB.append( tmp, group.resultInfo ) ;
         }
         if ( _changedGroupMask & CLS_IDX_MASK_OPINFO )
         {
            ossSnprintf( tmp, 64, "%s.$%u.%s",
                         FIELD_NAME_GROUPS, _i, FIELD_NAME_OPINFO ) ;
            setB.append( tmp, group.opInfo.c_str() ) ;
         }
         if ( _changedGroupMask & CLS_IDX_MASK_RETRYCNT )
         {
            ossSnprintf( tmp, 64, "%s.$%u.%s",
                         FIELD_NAME_GROUPS, _i, FIELD_NAME_RETRY_COUNT ) ;
            setB.append( tmp, group.retryCnt ) ;
         }
         if ( _changedGroupMask & CLS_IDX_MASK_PROGRESS )
         {
            ossSnprintf( tmp, 64, "%s.$%u.%s",
                         FIELD_NAME_GROUPS, _i, FIELD_NAME_PROGRESS ) ;
            setB.append( tmp, group.progress ) ;

            ossSnprintf( tmp, 64, "%s.$%u.%s",
                         FIELD_NAME_GROUPS, _i, FIELD_NAME_SPEED ) ;
            setB.append( tmp, (INT64)group.speed ) ;

            ossSnprintf( tmp, 64, "%s.$%u.%s",
                         FIELD_NAME_GROUPS, _i, FIELD_NAME_TIMESPENT ) ;
            setB.append( tmp, group.timeSpent ) ;

            ossSnprintf( tmp, 64, "%s.$%u.%s",
                         FIELD_NAME_GROUPS, _i, FIELD_NAME_TIMELEFT ) ;
            setB.append( tmp, group.timeLeft ) ;
         }
         if ( _changedGroupMask & CLS_IDX_MASK_TOTALREC )
         {
            ossSnprintf( tmp, 64, "%s.$%u.%s",
                         FIELD_NAME_GROUPS, _i, FIELD_NAME_TOTAL_RECORDS ) ;
            setB.append( tmp, (INT64)group.totalRecNum ) ;
         }
         if ( _changedGroupMask & CLS_IDX_MASK_PCSEDREC )
         {
            ossSnprintf( tmp, 64, "%s.$%u.%s",
                         FIELD_NAME_GROUPS, _i, FIELD_NAME_PROCESSED_RECORDS ) ;
            setB.append( tmp, (INT64)group.pcsedRecNum ) ;
         }
      }
   }

   void _clsIdxTask::_toChangeGroupObj( BSONObjBuilder& matcherB,
                                        BSONObjBuilder& setB )
   {
      if ( _changedMask & CLS_IDX_MASK_GROUPS )
      {
         BSONArrayBuilder arr( setB.subarrayStart( CAT_GROUPS_NAME ) ) ;
         for( MAP_GROUP_INFO_IT it = _mapGroupInfo.begin() ;
              it != _mapGroupInfo.end() ; ++it )
         {
            const _clsIdxTaskGroupUnit& group = it->second ;
            BSONObjBuilder b ;
            b.append( FIELD_NAME_GROUPNAME,  group.groupName.c_str() ) ;
            b.append( FIELD_NAME_STATUS,     group.status ) ;
            b.append( FIELD_NAME_STATUSDESC, clsTaskStatusStr(group.status) ) ;
            b.append( FIELD_NAME_RESULTCODE, group.resultCode ) ;
            b.append( FIELD_NAME_RESULTCODEDESC,
                      CLS_TASK_STATUS_FINISH == group.status ?
                      getErrDesp( group.resultCode ) : "" ) ;
            b.append( FIELD_NAME_RESULTINFO,        group.resultInfo ) ;
            b.append( FIELD_NAME_OPINFO,            group.opInfo.c_str() ) ;
            b.append( FIELD_NAME_RETRY_COUNT,       group.retryCnt ) ;
            b.append( FIELD_NAME_PROGRESS,          group.progress ) ;
            b.append( FIELD_NAME_SPEED,             (INT64)group.speed ) ;
            b.append( FIELD_NAME_TIMESPENT,         group.timeSpent ) ;
            b.append( FIELD_NAME_TIMELEFT,          group.timeLeft ) ;
            b.append( FIELD_NAME_TOTAL_RECORDS,     (INT64)group.totalRecNum ) ;
            b.append( FIELD_NAME_PROCESSED_RECORDS, (INT64)group.pcsedRecNum ) ;
            arr.append( b.obj() ) ;
         }
         arr.done() ;
      }
   }

   INT32 _clsIdxTask::_toChangedObj( const clsIdxTaskGroupUnit* group,
                                     const clsSubTaskUnit* subTask,
                                     BSONObj& matcher,
                                     BSONObj& updator )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder matcherBuilder, updatorBuilder, setBuilder ;

         _toChangeOtherObj( matcherBuilder, setBuilder ) ;
         if ( group )
         {
            _toChangeGroupObj( matcherBuilder, setBuilder, *group ) ;
         }
         if ( subTask )
         {
            _toChangeSubtaskObj( matcherBuilder, setBuilder, *subTask ) ;
         }

         BSONObj setObj = setBuilder.done() ;
         if ( !setObj.isEmpty() )
         {
            updatorBuilder.append( "$set", setObj ) ;
         }

         if ( _changedMask & CLS_IDX_MASK_PULL_GROUP )
         {
            updatorBuilder.append( "$pull_by",
                                   BSON( FIELD_NAME_GROUPS <<
                                         BSON( FIELD_NAME_GROUPNAME <<
                                               _pullGroupName.c_str() ) ) ) ;
         }
         if ( _changedMask & CLS_IDX_MASK_PULL_SUBTASK )
         {
            updatorBuilder.append( "$pull_by",
                                   BSON( FIELD_NAME_SUBTASKS <<
                                         BSON( FIELD_NAME_TASKID <<
                                               (INT64)_pullSubTaskID ) ) ) ;
         }
         if ( _changedMask & CLS_IDX_MASK_PUSH_GROUP )
         {
            clsIdxTaskGroupUnit oneGroup ;
            oneGroup.groupName = _pushGroupName ;
            updatorBuilder.append( "$push", BSON( FIELD_NAME_GROUPS <<
                                                  oneGroup.toBson() ) ) ;
         }

         updator = updatorBuilder.obj() ;
         matcher = matcherBuilder.obj() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return rc ;
   }

   INT32 _clsIdxTask::_toChangedObj( BSONObj& matcher, BSONObj& updator )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder matcherBuilder, updatorBuilder, setBuilder ;

         _toChangeOtherObj( matcherBuilder, setBuilder ) ;
         _toChangeGroupObj( matcherBuilder, setBuilder ) ;

         updatorBuilder.append( "$set", setBuilder.done() ) ;
         if ( _changedMask & CLS_IDX_MASK_PULL_GROUP )
         {
            updatorBuilder.append( "$pull_by",
                                   BSON( FIELD_NAME_GROUPS <<
                                         BSON( FIELD_NAME_GROUPNAME <<
                                               _pullGroupName.c_str() ) ) ) ;
         }
         if ( _changedMask & CLS_IDX_MASK_PULL_SUBTASK )
         {
            updatorBuilder.append( "$pull_by",
                                   BSON( FIELD_NAME_SUBTASKS <<
                                         BSON( FIELD_NAME_TASKID <<
                                               (INT64)_pullSubTaskID ) ) ) ;
         }

         updator = updatorBuilder.obj() ;
         matcher = matcherBuilder.obj() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return rc ;
   }

   /*
      _clsCreateIdxTask : implement
   */
   INT32 _clsCreateIdxTask::sortBufSize() const
   {
      return _sortBufferSize ;
   }

   const BSONObj& _clsCreateIdxTask::indexDef() const
   {
      return _indexDef ;
   }

   INT32 _clsCreateIdxTask::globalIdxCL( const CHAR *&clName,
                                         UINT64 &clUniqID ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONElement ele = _indexDef.getField( IXM_FIELD_NAME_GLOBAL_OPTION ) ;
         if ( Object == ele.type() )
         {
            BSONObj option = ele.embeddedObject() ;

            ele = option.getField( FIELD_NAME_COLLECTION ) ;
            if ( String == ele.type() )
            {
               clName = ele.valuestr() ;
            }

            ele = option.getField( FIELD_NAME_CL_UNIQUEID ) ;
            if ( ele.isNumber() )
            {
               clUniqID = ele.numberLong() ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         rc = SDB_SYS ;
      }

      return rc ;
   }

   INT32 _clsCreateIdxTask::addGlobalOpt2Def( const CHAR* globalIdxCLName,
                                              utilCLUniqueID globalIdxCLUniqID )
   {
     /* IndexDef:
      * { name: 'a', key: {a: 1}, ... }
      * ==>
      * { name: 'a', key: {a: 1}, ... , GlobalOption: { Collection: xx,
      *                                                 CLUniqueID: xx } }
      */
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder builder ;
         builder.appendElements( _indexDef ) ;
         BSONObjBuilder sub( builder.subobjStart( IXM_FIELD_NAME_GLOBAL_OPTION ) ) ;
         sub.append( FIELD_NAME_COLLECTION, globalIdxCLName ) ;
         sub.append( FIELD_NAME_CL_UNIQUEID, (INT64)globalIdxCLUniqID ) ;
         sub.done() ;
         _indexDef = builder.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         rc = SDB_SYS ;
      }

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCRTIDXTASK_INITTASK, "_clsCreateIdxTask::initTask" )
   INT32 _clsCreateIdxTask::initTask( const CHAR *clFullName,
                                      utilCLUniqueID clUniqID,
                                      const BSONObj &index,
                                      UINT64 idxUniqID,
                                      const vector<string> &groupList,
                                      INT32 sortBufSize,
                                      UINT64 mainTaskID )
   {
      SDB_ASSERT( clFullName, "Invalid collection name" ) ;
      PD_TRACE_ENTRY( SDB_CLSCRTIDXTASK_INITTASK ) ;

      INT32 rc = SDB_OK ;

      try
      {
         // collection
         ossStrncpy( _clFullName, clFullName, DMS_COLLECTION_FULL_NAME_SZ ) ;
         _clUniqueID = clUniqID ;

         // idxDef: { "name": "aIdx", "key": { "a": 1 }, ... }
         BSONObjBuilder builder ;
         if ( !index.hasField( DMS_ID_KEY_NAME ) )
         {
            builder.append( DMS_ID_KEY_NAME, OID::gen() ) ;
         }
         builder.append( FIELD_NAME_UNIQUEID, (INT64)idxUniqID ) ;
         builder.appendElementsUnique( index ) ;
         _indexDef = builder.obj() ;

         BSONElement ele = _indexDef.getField( IXM_FIELD_NAME_NAME ) ;
         PD_CHECK ( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in index def[%s]",
                    IXM_FIELD_NAME_NAME, _indexDef.toString().c_str() ) ;
         ossStrncpy( _indexName, ele.valuestr(), IXM_INDEX_NAME_SIZE ) ;

         _sortBufferSize = sortBufSize ;

         // group list
         for(  vector<string>::const_iterator it = groupList.begin();
               it != groupList.end() ;
               it++ )
         {
            clsIdxTaskGroupUnit oneGroup ;
            oneGroup.groupName = it->c_str() ;
            _mapGroupInfo[ oneGroup.groupName ]= oneGroup ;
         }

         _totalGroups = groupList.size() ;

         // other
         _mainTaskID = mainTaskID ;

         _makeName() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CLSCRTIDXTASK_INITTASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCRTIDXTASK_INITMAINTASK, "_clsCreateIdxTask::initMainTask" )
   INT32 _clsCreateIdxTask::initMainTask( const CHAR *clFullName,
                                          utilCLUniqueID clUniqID,
                                          const BSONObj &index,
                                          UINT64 idxUniqID,
                                          const ossPoolSet<ossPoolString> &groupList,
                                          const ossPoolVector<UINT64> &subTaskList )
   {
      SDB_ASSERT( clFullName, "Invalid collection name" ) ;
      PD_TRACE_ENTRY( SDB_CLSCRTIDXTASK_INITMAINTASK ) ;

      INT32 rc = SDB_OK ;

      try
      {
         // collection
         ossStrncpy( _clFullName, clFullName, DMS_COLLECTION_FULL_NAME_SZ ) ;
         _clUniqueID = clUniqID ;

         // idxDef: { "name": "aIdx", "key": { "a": 1 }, ... }
         BSONObjBuilder builder ;
         if ( !index.hasField( DMS_ID_KEY_NAME ) )
         {
            builder.append( DMS_ID_KEY_NAME, OID::gen() ) ;
         }
         builder.append( FIELD_NAME_UNIQUEID, (INT64)idxUniqID ) ;
         builder.appendElementsUnique( index ) ;
         _indexDef = builder.obj() ;

         BSONElement ele = _indexDef.getField( IXM_FIELD_NAME_NAME ) ;
         PD_CHECK ( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in index def[%s]",
                    IXM_FIELD_NAME_NAME, _indexDef.toString().c_str() ) ;
         ossStrncpy( _indexName, ele.valuestr(), IXM_INDEX_NAME_SIZE ) ;

         // group list
         for(  ossPoolSet<ossPoolString>::const_iterator it = groupList.begin();
               it != groupList.end() ;
               it++ )
         {
            clsIdxTaskGroupUnit oneGroup ;
            oneGroup.groupName = it->c_str() ;
            _mapGroupInfo[ oneGroup.groupName ]= oneGroup ;
         }

         _totalGroups = groupList.size() ;

         // sub-task list
         _isMainTask = TRUE ;

         for(  ossPoolVector<UINT64>::const_iterator it = subTaskList.begin() ;
               it != subTaskList.end() ;
               it++ )
         {
            UINT64 taskID = *it ;
            _clsSubTaskUnit oneSubTask( taskID, CLS_TASK_CREATE_IDX ) ;
            _mapSubTask[ taskID ] = oneSubTask ;
         }

         _totalTasks = subTaskList.size() ;

         // other
         _makeName() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CLSCRTIDXTASK_INITMAINTASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _clsCreateIdxTask::commandName() const
   {
      return CMD_NAME_CREATE_INDEX ;
   }

   INT32 _clsCreateIdxTask::checkConflictWithExistTask( const _clsTask *pExistTask )
   {
      INT32 rc = SDB_OK ;

      if ( 0 != ossStrcmp( collectionName(), pExistTask->collectionName() ) )
      {
         goto done ;
      }

      if ( CLS_TASK_CREATE_IDX == pExistTask->taskType() )
      {
         clsCreateIdxTask* pExistIdxTask = (clsCreateIdxTask*)pExistTask ;
         if ( 0 == ossStrcmp( indexName(), pExistIdxTask->indexName() ) )
         {
            if ( ixmIsSameDef( pExistIdxTask->indexDef(), indexDef(), TRUE ) )
            {
               rc = SDB_IXM_CREATING ;
               PD_LOG_MSG( PDERROR,
                           "The same index '%s' is creating in task[%llu]",
                           indexName(), pExistIdxTask->taskID() ) ;
               goto error ;
            }
            else
            {
               rc = SDB_IXM_SAME_NAME_CREATING ;
               PD_LOG_MSG( PDERROR,
                           "An index '%s' which has the same name but with "
                           "different definition is creating in task[%llu]",
                           pExistIdxTask->indexName(), pExistIdxTask->taskID() ) ;
               goto error ;
            }
         }
         else
         {
            if ( ixmIsSameDef( pExistIdxTask->indexDef(), indexDef() ) )
            {
               rc = SDB_IXM_COVER_CREATING ;
               PD_LOG_MSG( PDERROR, "An index '%s' which "
                           "can cover this scene is creating in task[%llu]",
                           pExistIdxTask->indexName(), pExistIdxTask->taskID() ) ;
               goto error ;
            }
         }
      }
      else if ( CLS_TASK_DROP_IDX == pExistTask->taskType() )
      {
         clsDropIdxTask* pExistIdxTask = (clsDropIdxTask*)pExistTask ;
         if ( 0 == ossStrcmp( indexName(), pExistIdxTask->indexName() ) )
         {
            rc = SDB_IXM_DROPPING ;
            PD_LOG_MSG( PDERROR,
                        "The index '%s' is dropping in task[%llu]",
                        indexName(), pExistIdxTask->taskID() ) ;
            goto error ;
         }
      }
      else if ( CLS_TASK_COPY_IDX == pExistTask->taskType() )
      {
         clsCopyIdxTask* pExistIdxTask = (clsCopyIdxTask*)pExistTask ;
         if ( pExistIdxTask->indexList().count( indexName() ) > 0 )
         {
            rc = SDB_IXM_CREATING ;
            PD_LOG_MSG( PDERROR,
                        "The index '%s' is creating in task[%llu]",
                        indexName(), pExistIdxTask->taskID() ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsCreateIdxTask::_init( const CHAR *objdata )
   {
      INT32 rc = SDB_OK ;

      if ( !objdata )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         BSONObj obj( objdata ) ;
         BSONElement ele ;

         ele = obj.getField ( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) ;
         PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error,
                    PDERROR, "Field[%s] invalid in task[%s]",
                    IXM_FIELD_NAME_SORT_BUFFER_SIZE, obj.toString().c_str() ) ;
         _sortBufferSize = ele.numberInt() ;

         ele = obj.getField ( IXM_FIELD_NAME_INDEX_DEF ) ;
         PD_CHECK ( ele.type() == Object , SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in task[%s]",
                    IXM_FIELD_NAME_INDEX_DEF, obj.toString().c_str() ) ;
         _indexDef = ele.Obj().getOwned() ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _clsCreateIdxTask::_toBson( BSONObjBuilder &builder )
   {
      try
      {
         builder.append( IXM_FIELD_NAME_INDEX_DEF,        _indexDef ) ;
         builder.append( IXM_FIELD_NAME_SORT_BUFFER_SIZE, _sortBufferSize ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
      }
   }

   /*
      _clsDropIdxTask : implement
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSDROPIDXTASK_INITTASK, "_clsDropIdxTask::initTask" )
   INT32 _clsDropIdxTask::initTask( const CHAR *clFullName,
                                    utilCLUniqueID clUniqID,
                                    const CHAR *indexName,
                                    const vector<string> &groupList,
                                    UINT64 mainTaskID )
   {
      SDB_ASSERT( clFullName, "Invalid collection name" ) ;
      PD_TRACE_ENTRY( SDB_CLSDROPIDXTASK_INITTASK ) ;

      INT32 rc = SDB_OK ;

      // collection
      ossStrncpy( _clFullName, clFullName, DMS_COLLECTION_FULL_NAME_SZ ) ;
      _clUniqueID = clUniqID ;

      // index
      ossStrncpy( _indexName, indexName, IXM_INDEX_NAME_SIZE ) ;

      // group list
      for(  vector<string>::const_iterator it = groupList.begin();
            it != groupList.end() ;
            ++it )
      {
         clsIdxTaskGroupUnit oneGroup ;
         oneGroup.groupName = it->c_str() ;
         try
         {
            _mapGroupInfo[ oneGroup.groupName ] = oneGroup ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      _totalGroups = groupList.size() ;

      // other
      _mainTaskID = mainTaskID ;

      _makeName() ;

   done:
      PD_TRACE_EXITRC( SDB_CLSDROPIDXTASK_INITTASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSDROPIDXTASK_INITMAINTASK, "_clsDropIdxTask::initMainTask" )
   INT32 _clsDropIdxTask::initMainTask( const CHAR *clFullName,
                                        utilCLUniqueID clUniqID,
                                        const CHAR *indexName,
                                        const ossPoolSet<ossPoolString> &groupList,
                                        const ossPoolVector<UINT64> &subTaskList )
   {
      SDB_ASSERT( clFullName, "Invalid collection name" ) ;
      PD_TRACE_ENTRY( SDB_CLSDROPIDXTASK_INITMAINTASK ) ;

      INT32 rc = SDB_OK ;

      // collection
      ossStrncpy( _clFullName, clFullName, DMS_COLLECTION_FULL_NAME_SZ ) ;
      _clUniqueID = clUniqID ;

      // index
      ossStrncpy( _indexName, indexName, IXM_INDEX_NAME_SIZE ) ;

      // group list
      for(  ossPoolSet<ossPoolString>::const_iterator it = groupList.begin();
            it != groupList.end() ;
            it++ )
      {
         clsIdxTaskGroupUnit oneGroup ;
         oneGroup.groupName = it->c_str() ;
         try
         {
            _mapGroupInfo[ oneGroup.groupName ] = oneGroup ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      _totalGroups = groupList.size() ;

      // sub-task list
      _isMainTask = TRUE ;

      for(  ossPoolVector<UINT64>::const_iterator it = subTaskList.begin();
            it != subTaskList.end() ;
            it++ )
      {
         UINT64 taskID = *it ;
         _clsSubTaskUnit oneSubTask( taskID, CLS_TASK_DROP_IDX ) ;
         _mapSubTask[ taskID ] = oneSubTask ;
      }

      _totalTasks = subTaskList.size() ;

      // other
      _makeName() ;

   done:
      PD_TRACE_EXITRC( SDB_CLSDROPIDXTASK_INITMAINTASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _clsDropIdxTask::commandName() const
   {
      return CMD_NAME_DROP_INDEX ;
   }

   INT32 _clsDropIdxTask::checkConflictWithExistTask( const _clsTask *pExistTask )
   {
      INT32 rc = SDB_OK ;

      if ( 0 != ossStrcmp( collectionName(), pExistTask->collectionName() ) )
      {
         goto done ;
      }

      if ( CLS_TASK_CREATE_IDX == pExistTask->taskType() )
      {
         clsCreateIdxTask* pExistIdxTask = (clsCreateIdxTask*)pExistTask ;
         if ( 0 == ossStrcmp( indexName(), pExistIdxTask->indexName() ) )
         {
            rc = SDB_IXM_CREATING ;
            PD_LOG_MSG( PDERROR,
                        "The same index '%s' is creating in task[%llu]",
                        indexName(), pExistIdxTask->taskID() ) ;
            goto error ;
         }
      }
      else if ( CLS_TASK_DROP_IDX == pExistTask->taskType() )
      {
         clsDropIdxTask* pExistIdxTask = (clsDropIdxTask*)pExistTask ;
         if ( 0 == ossStrcmp( indexName(), pExistIdxTask->indexName() ) )
         {
            rc = SDB_IXM_DROPPING ;
            PD_LOG_MSG( PDERROR,
                        "The index '%s' is dropping in task[%llu]",
                        indexName(), pExistIdxTask->taskID() ) ;
            goto error ;
         }
      }
      else if ( CLS_TASK_COPY_IDX == pExistTask->taskType() )
      {
         clsCopyIdxTask* pExistIdxTask = (clsCopyIdxTask*)pExistTask ;
         if ( pExistIdxTask->indexList().count( indexName() ) > 0 )
         {
            rc = SDB_IXM_CREATING ;
            PD_LOG_MSG( PDERROR,
                        "The index '%s' is creating in task[%llu]",
                        indexName(), pExistIdxTask->taskID() ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDropIdxTask::_init( const CHAR *objdata )
   {
      return SDB_OK ;
   }

   void _clsDropIdxTask::_toBson( BSONObjBuilder &builder )
   {
   }

   /*
      _clsCopyIdxTask : implement
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCOPYIDXTASK_INITMAINTASK, "_clsCopyIdxTask::initMainTask" )
   INT32 _clsCopyIdxTask::initMainTask( const CHAR *clFullName,
                                        utilCLUniqueID clUniqID,
                                        const ossPoolSet<ossPoolString> &subCLList,
                                        const ossPoolSet<ossPoolString> &indexList,
                                        const ossPoolSet<ossPoolString> &groupList,
                                        const ossPoolVector<UINT64> &subTaskList )
   {
      SDB_ASSERT( clFullName, "Invalid collection name" ) ;
      PD_TRACE_ENTRY( SDB_CLSCOPYIDXTASK_INITMAINTASK ) ;

      INT32 rc = SDB_OK ;

      // collection index
      ossStrncpy( _clFullName, clFullName, DMS_COLLECTION_FULL_NAME_SZ ) ;
      _clUniqueID = clUniqID ;

      _subCLList = subCLList ;
      _indexList = indexList ;

      // group list
      for(  ossPoolSet<ossPoolString>::const_iterator it = groupList.begin();
            it != groupList.end() ;
            it++ )
      {
         clsIdxTaskGroupUnit oneGroup ;
         oneGroup.groupName = it->c_str() ;
         try
         {
            _mapGroupInfo[ oneGroup.groupName ] = oneGroup ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      _totalGroups = groupList.size() ;

      // sub-task list
      _isMainTask = TRUE ;

      for(  ossPoolVector<UINT64>::const_iterator it = subTaskList.begin();
            it != subTaskList.end() ;
            it++ )
      {
         UINT64 taskID = *it ;
         _clsSubTaskUnit oneSubTask( taskID, CLS_TASK_CREATE_IDX ) ;
         try
         {
            _mapSubTask[ taskID ] = oneSubTask ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      _totalTasks = subTaskList.size() ;

      // other
      _makeName() ;

   done:
      PD_TRACE_EXITRC( SDB_CLSCOPYIDXTASK_INITMAINTASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsCopyIdxTask::_init( const CHAR *objdata )
   {
      INT32 rc = SDB_OK ;

      if ( !objdata )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         BSONObj jobObj ( objdata ) ;

         BSONElement ele = jobObj.getField ( FIELD_NAME_COPYTO ) ;
         PD_CHECK ( Array == ele.type(), SDB_INVALIDARG, error,
                    PDERROR, "Field[%s] invalid in task[%s]",
                    FIELD_NAME_COPYTO, jobObj.toString().c_str() ) ;

         BSONObjIterator it( ele.embeddedObject() ) ;
         while( it.more() )
         {
            BSONElement e = it.next() ;
            PD_CHECK( e.type() == String, SDB_INVALIDARG, error,
                      PDERROR, "Field invalid in task[%s]",
                      jobObj.toString().c_str() ) ;
            _subCLList.insert( e.valuestr() ) ;
         }

         ele = jobObj.getField ( FIELD_NAME_INDEXNAMES ) ;
         PD_CHECK ( Array == ele.type(), SDB_INVALIDARG, error,
                    PDERROR, "Field[%s] invalid in task[%s]",
                    FIELD_NAME_INDEXNAMES, jobObj.toString().c_str() ) ;

         BSONObjIterator itr( ele.embeddedObject() ) ;
         while( itr.more() )
         {
            BSONElement e = itr.next() ;
            PD_CHECK( e.type() == String, SDB_INVALIDARG, error,
                      PDERROR, "Field invalid in task[%s]",
                      jobObj.toString().c_str() ) ;
            _indexList.insert( e.valuestr() ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _clsCopyIdxTask::_toBson( BSONObjBuilder &builder )
   {
      try
      {

         BSONArrayBuilder arrB1( builder.subarrayStart( FIELD_NAME_COPYTO ) ) ;
         for ( ossPoolSet<ossPoolString>::iterator it = _subCLList.begin() ;
               it != _subCLList.end() ; ++it )
         {
            arrB1.append( it->c_str() ) ;
         }
         arrB1.done() ;

         BSONArrayBuilder arrB2( builder.subarrayStart(FIELD_NAME_INDEXNAMES) ) ;
         for ( ossPoolSet<ossPoolString>::iterator it = _indexList.begin() ;
               it != _indexList.end() ; ++it )
         {
            arrB2.append( it->c_str() ) ;
         }
         arrB2.done() ;

      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
      }
   }

   const ossPoolSet<ossPoolString>& _clsCopyIdxTask::indexList() const
   {
      return _indexList ;
   }

   const CHAR* _clsCopyIdxTask::commandName() const
   {
      return CMD_NAME_COPY_INDEX ;
   }

   INT32 _clsCopyIdxTask::checkConflictWithExistTask( const _clsTask *pExistTask )
   {
      INT32 rc = SDB_OK ;

      if ( 0 != ossStrcmp( collectionName(), pExistTask->collectionName() ) )
      {
         goto done ;
      }

      if ( CLS_TASK_CREATE_IDX == pExistTask->taskType() )
      {
         clsCreateIdxTask* pExistIdxTask = (clsCreateIdxTask*)pExistTask ;
         if ( _indexList.count( pExistIdxTask->indexName() ) > 0 )
         {
            rc = SDB_IXM_CREATING ;
            PD_LOG_MSG( PDERROR,
                        "The same index '%s' is creating in task[%llu]",
                        indexName(), pExistIdxTask->taskID() ) ;
            goto error ;
         }
      }
      else if ( CLS_TASK_DROP_IDX == pExistTask->taskType() )
      {
         clsDropIdxTask* pExistIdxTask = (clsDropIdxTask*)pExistTask ;
         if ( _indexList.count( pExistIdxTask->indexName() ) > 0 )
         {
            rc = SDB_IXM_DROPPING ;
            PD_LOG_MSG( PDERROR,
                        "The index '%s' is dropping in task[%llu]",
                        indexName(), pExistIdxTask->taskID() ) ;
            goto error ;
         }
      }
      else if ( CLS_TASK_COPY_IDX == pExistTask->taskType() )
      {
         // do nothing, copy index check by sub-tasks
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

