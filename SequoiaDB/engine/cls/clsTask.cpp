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


using namespace bson ;

namespace engine
{
   /*
   _clsTaskMgr : implement
   */
   _clsTaskMgr::_clsTaskMgr ( UINT64 maxTaskID )
   {
      _taskID = CLS_INVALID_TASKID ;
      _maxID  = maxTaskID ;
   }

   _clsTaskMgr::~_clsTaskMgr ()
   {
      std::map<UINT64, _clsTask*>::iterator it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         SDB_OSS_DEL it->second ;
         ++it ;
      }
      _taskMap.clear() ;
   }

   UINT64 _clsTaskMgr::getTaskID ()
   {
      ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;
      ++_taskID ;
      if ( CLS_INVALID_TASKID == _taskID ||
           ( CLS_INVALID_TASKID != _maxID && _taskID > _maxID )  )
      {
         _taskID = CLS_INVALID_TASKID + 1 ;
      }
      return _taskID ;
   }

   void _clsTaskMgr::setTaskID( UINT64 taskID )
   {
      ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;
      _taskID = taskID ;
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

      std::map<UINT64, _clsTask*>::iterator it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         clsTask *pTask = it->second ;
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

      std::map<UINT64, _clsTask*>::iterator it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         clsTask *pTask = it->second ;
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

      std::map<UINT64, _clsTask*>::iterator it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         clsTask *pTask = it->second ;
         if ( pTask->collectionSpaceName() &&
              0 == ossStrcmp( pCSName, pTask->collectionSpaceName() ) )
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

   PD_TRACE_DECLARE_FUNCTION ( SDB__CLSTKMGR_ADDTK, "_clsTaskMgr::addTask" )
   INT32 _clsTaskMgr::addTask ( _clsTask * pTask, UINT64 taskID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSTKMGR_ADDTK ) ;
      _clsTask *indexTask = NULL ;

      if ( CLS_INVALID_TASKID == taskID )
      {
         taskID = pTask->taskID() ;
      }

      ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;

      std::map<UINT64, _clsTask*>::iterator it = _taskMap.begin () ;
      while ( it != _taskMap.end() )
      {
         indexTask = it->second ;

         if ( taskID == it->first ||
              ( pTask->taskType() == indexTask->taskType() &&
                pTask->taskID() == indexTask->taskID() ) ||
              pTask->muteXOn( indexTask ) || indexTask->muteXOn( pTask ) )
         {
            PD_LOG ( PDWARNING, "Exist task[%lld,%s] mutex with new task[%lld,%s]",
                     indexTask->taskID(), indexTask->taskName(),
                     pTask->taskID(), pTask->taskName() ) ;
            rc = SDB_CLS_MUTEX_TASK_EXIST ;
            goto error ;
         }
         ++it ;
      }
      _taskMap[ taskID ] = pTask ;
   done:
      PD_TRACE_EXITRC ( SDB__CLSTKMGR_ADDTK, rc ) ;
      return rc ;
   error:
      SDB_OSS_DEL pTask ;
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__CLSTKMGR_RVTK1, "_clsTaskMgr::removeTask" )
   INT32 _clsTaskMgr::removeTask ( UINT64 taskID )
   {
      PD_TRACE_ENTRY ( SDB__CLSTKMGR_RVTK1 ) ;
      ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;
      std::map<UINT64, _clsTask*>::iterator it = _taskMap.find ( taskID ) ;
      if ( it != _taskMap.end() )
      {
         SDB_OSS_DEL it->second ;
         _taskMap.erase ( it ) ;
         _taskEvent.signal() ;
      }

      PD_TRACE_EXIT ( SDB__CLSTKMGR_RVTK1 ) ;
      return SDB_OK ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__CLSTKMGR_RVTK2, "_clsTaskMgr::removeTask" )
   INT32 _clsTaskMgr::removeTask ( _clsTask * pTask )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSTKMGR_RVTK2 ) ;
      rc = removeTask ( pTask->taskID () ) ;
      PD_TRACE_EXITRC ( SDB__CLSTKMGR_RVTK2, rc ) ;
      return rc ;
   }

   _clsTask* _clsTaskMgr::findTask ( UINT64 taskID )
   {
      ossScopedLock lock ( &_taskLatch, SHARED ) ;
      std::map<UINT64, _clsTask*>::iterator it = _taskMap.find ( taskID ) ;
      if ( it != _taskMap.end() )
      {
         return it->second ;
      }
      return NULL ;
   }

   void _clsTaskMgr::stopTask( UINT64 taskID )
   {
      ossScopedLock lock ( &_taskLatch, SHARED ) ;
      std::map<UINT64, _clsTask*>::iterator it = _taskMap.find ( taskID ) ;
      if ( it != _taskMap.end() )
      {
         return it->second->setStatus( CLS_TASK_STATUS_CANCELED ) ;
      }
   }

   string _clsTaskMgr::dumpTasks( CLS_TASK_TYPE type )
   {
      string taskStr ;

      ossScopedLock lock ( &_taskLatch, SHARED ) ;

      std::map<UINT64, _clsTask*>::iterator it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         clsTask *pTask = it->second ;
         if ( CLS_TASK_UNKNOW == type ||
              type == pTask->taskType() )
         {
            taskStr += "[ taskName: " ;
            taskStr += pTask->taskName() ? pTask->taskName() : "" ;
            taskStr += " collectionName: " ;
            taskStr += pTask->collectionName() ? pTask->collectionName() : "" ;
            taskStr += " ]" ;
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

   CLS_TASK_TYPE _clsDummyTask::taskType() const
   {
      return CLS_TASK_UNKNOW ;
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

   /*
   _clsSplitTask : implement
   */
   _clsSplitTask::_clsSplitTask ( UINT64 taskID )
   : _clsTask ( taskID )
   {
      _sourceID = 0 ;
      _dstID = 0 ;
      _status = CLS_TASK_STATUS_READY ;
      _taskType = CLS_TASK_SPLIT ;
      _percent  = 0.0 ;
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

      _clFullName    = clFullName ;
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

      BSONObj groupUpBound ;
      BSONObj allUpbound ;
      rc = cataSet.getGroupUpBound( sourceID, groupUpBound ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get group up bound, rc: %d", rc ) ;

      rc = cataSet.getGroupUpBound( 0, allUpbound ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get all up bound, rc: %d", rc ) ;

      PD_CHECK( !_splitKeyObj.isEmpty(), SDB_INVALIDARG, error, PDERROR,
                "Split begin key can't be empty" ) ;

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

      if ( _splitEndKeyObj.isEmpty() )
      {
         _splitEndKeyObj = groupUpBound.getOwned() ;
      }

      if ( 0 == _splitEndKeyObj.woCompare( allUpbound, BSONObj(), false ) )
      {
         _splitEndKeyObj = BSONObj() ;
      }

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

         ele = jobObj.getField ( CAT_TASKID_NAME ) ;
         PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                    "Field[%s] invalid in split task[%s]", CAT_TASKID_NAME,
                    jobObj.toString().c_str() ) ;
         _taskID = (UINT64)ele.numberLong() ;

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

   BSONObj _clsSplitTask::toBson( UINT32 mask ) const
   {
      BSONObjBuilder builder ;

      if ( mask & CLS_SPLIT_MASK_ID )
      {
         builder.append( CAT_TASKID_NAME, (INT64)_taskID ) ;
      }
      if ( mask & CLS_SPLIT_MASK_TYPE )
      {
         builder.append( CAT_TASKTYPE_NAME, (INT32)_taskType ) ;
      }
      if ( mask & CLS_SPLIT_MASK_STATUS )
      {
         builder.append( CAT_STATUS_NAME, (INT32)_status ) ;
      }
      if ( mask & CLS_SPLIT_MASK_CLNAME )
      {
         builder.append( CAT_COLLECTION_NAME, _clFullName ) ;
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

      return builder.obj() ;
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

         if ( rangeNum < splitNum )
         {
            splitNum -= rangeNum ;
         }
         else if ( rangeNum == splitNum )
         {
            bKey = cataItem->getUpBound().getOwned() ;
            break ;
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

   CLS_TASK_TYPE _clsSplitTask::taskType () const
   {
      return _taskType ;
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

         if ( 0 != ossStrcmp ( clFullName (), pOtherSplit->clFullName () ) )
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

   const CHAR* _clsSplitTask::clFullName () const
   {
      return _clFullName.c_str() ;
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

}

