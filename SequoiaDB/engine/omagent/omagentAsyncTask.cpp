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

   Source File Name = omagentBackgroundCmd.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/09/2017  HJW Initial Draft

   Last Changed =

*******************************************************************************/

#include "omagentJob.hpp"
#include "omagentAsyncTask.hpp"
#include "omagentCmdBase.hpp"
#include "omagentMgr.hpp"

using namespace bson ;

namespace engine
{
   #define OMA_STR_FIELD_PLAN                "Plan"
   #define OMA_STR_STEP_ARG                  "SYS_STEP"
   #define OMA_STR_STEP_GENERATE_PLAN        "Generate plan"
   #define OMA_STR_STEP_DOIT                 "Doit"
   #define OMA_STR_STEP_CHECK_RESULT         "Check result"
   #define OMA_STR_STEP_ROLLBACK             "Rollback"

   #define OMA_WAIT_OMSVC_RES_TIMEOUT        ( 1 * OSS_ONE_SEC )
   #define OMA_WAIT_SUB_TASK_NOTIFY_TIMEOUT  ( 3 * OSS_ONE_SEC )
   #define MAX_THREAD_NUM                    10

   INT32 _addStepVar( _omaCommand* cmd, const CHAR* value )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;

      ss << "var "OMA_STR_STEP_ARG" = \"" << value << "\";" ;

      rc = cmd->addUserDefineVar( ss.str().c_str() ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "Failed to add user define variable, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* omagentGetEDUInfoSafe( EDU_INFO_TYPE type )
   {
      const CHAR *info = pmdGetThreadEDUCB()->getInfo( type ) ;
      if ( NULL == info )
      {
         return "" ;
      }
      return info ;
   }

   _omaAsyncTask::_omaAsyncTask( INT64 taskID, const CHAR* command )
         : _omaTask( taskID ),
           _errno( SDB_OK ),
           _isSetErrInfo( FALSE ),
           _planTaskNum( 0 )
   {
      SDB_ASSERT( command != NULL, "omagent command can't not be null" ) ;
      _taskID  = taskID ;
      _command = command ;
      _taskName = command ;
   }

   _omaAsyncTask::~_omaAsyncTask()
   {
   }

   INT32 _omaAsyncTask::init( const BSONObj &info, void *ptr )
   {
      BSONObj condition = BSON( OMA_FIELD_TASKID<< "" <<
                                OMA_FIELD_INFO << "" <<
                                OMA_FIELD_ERRNO << "" <<
                                OMA_FIELD_DETAIL << "" <<
                                OMA_FIELD_PROGRESS << "" <<
                                OMA_FIELD_RESULTINFO << "" ) ;

      _taskInfo = info.filterFieldsUndotted( condition, TRUE ) ;

      return SDB_OK ;
   }

   INT32 _omaAsyncTask::doit()
   {
      INT32 rc = SDB_OK ;
      BSONObj planInfo ;

      rc = _generateTaskPlan( planInfo ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "Failed to generate task plan, rc=%d", rc ) ;
         goto error ;
      }

      rc = _execTaskPlan( planInfo ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "Failed to exec task plan, rc=%d", rc ) ;
         goto error ;
      }

   done:
      sdbGetOMAgentMgr()->submitTaskInfo( _taskID ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _omaAsyncTask::setTaskRunning( _omaCommand* cmd,
                                        const BSONObj& itemInfo )
   {
      INT32 rc = SDB_OK ;
      BSONObj resultInfo = itemInfo.getObjectField( OMA_FIELD_RESULTINFO ) ;
      ossScopedLock lock( &_planLatch, EXCLUSIVE ) ;

      rc = cmd->setRuningStatus( resultInfo, _taskInfo ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "Failed to convert command result, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omaAsyncTask::updateTaskInfo( _omaCommand* cmd,
                                        const BSONObj& itemInfo )
   {
      INT32 rc = SDB_OK ;
      ossScopedLock lock( &_planLatch, EXCLUSIVE ) ;

      rc = cmd->convertResult( itemInfo, _taskInfo ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "Failed to convert command result, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _omaAsyncTask::notifyUpdateProgress()
   {
      ossScopedLock lock( &_planLatch, EXCLUSIVE ) ;

      _planEvent.signal() ;

      --_planTaskNum ;
   }

   INT32 _omaAsyncTask::updateProgressToOM()
   {
      INT32 rc     = SDB_OK ;
      INT32 retRc  = SDB_OK ;
      UINT64 reqID = 0 ;
      omAgentMgr *pOmaMgr = sdbGetOMAgentMgr() ;
      _pmdEDUCB* cb = pmdGetThreadEDUCB () ;
      ossScopedLock lock( &_planLatch, EXCLUSIVE ) ;
      ossAutoEvent updateEvent ;
      BSONObj progressInfo ;
      BSONObj condition ;
      BSONObjBuilder progressBuilder ;

      if( _taskStatus == OMA_TASK_STATUS_FINISH )
      {
         if( _isSetErrInfo == TRUE )
         {
            condition = BSON( OMA_FIELD_RESULTINFO << "" ) ;
         }
         else
         {
            condition = BSON( OMA_FIELD_ERRNO << "" <<
                              OMA_FIELD_DETAIL << "" <<
                              OMA_FIELD_RESULTINFO << "" ) ;
         }
      }
      else
      {
         if( _isSetErrInfo == TRUE )
         {
            condition = BSON( OMA_FIELD_PROGRESS << "" <<
                              OMA_FIELD_RESULTINFO << "" ) ;

         }
         else
         {
            condition = BSON( OMA_FIELD_ERRNO << "" <<
                              OMA_FIELD_DETAIL << "" <<
                              OMA_FIELD_PROGRESS << "" <<
                              OMA_FIELD_RESULTINFO << "" ) ;
         }
      }

      progressInfo = _taskInfo.filterFieldsUndotted( condition, TRUE ) ;

      progressBuilder.appendNumber( OMA_FIELD_TASKID, _taskID ) ;
      progressBuilder.appendNumber( OMA_FIELD_STATUS, _taskStatus ) ;
      progressBuilder.append( OMA_FIELD_STATUSDESC,
                              getTaskStatusDesc( _taskStatus ) ) ;
      if( _taskStatus == OMA_TASK_STATUS_FINISH )
      {
         progressBuilder.appendNumber( OMA_FIELD_PROGRESS, 100 ) ;
      }

      if( _isSetErrInfo == TRUE )
      {
         progressBuilder.appendNumber( OMA_FIELD_ERRNO, _errno ) ;
         progressBuilder.append( OMA_FIELD_DETAIL, _detail ) ;
      }

      progressBuilder.appendElements( progressInfo ) ;

      progressInfo = progressBuilder.obj() ;

      reqID = pOmaMgr->getRequestID() ;
      pOmaMgr->registerTaskEvent( reqID, &updateEvent ) ;

      do
      {
         pOmaMgr->sendUpdateTaskReq( reqID, &progressInfo ) ;
         if( updateEvent.wait( OMA_WAIT_OMSVC_RES_TIMEOUT, &retRc ) )
         {
            // try to send update task request again
         }
         else
         {
            if( retRc == SDB_OM_TASK_NOT_EXIST )
            {
               PD_LOG( PDERROR, "Failed to update task[%lld]'s progress "
                       "with requestID[%lld], rc = %d",
                       _taskID, reqID, retRc ) ;
               rc = retRc ;
               goto error ;
            }
            else if( retRc )
            {
               PD_LOG( PDWARNING, "Retry to update task[%lld]'s progress "
                       "with requestID[%lld], rc = %d",
                       _taskID, reqID, retRc ) ;
            }
            else
            {
               PD_LOG( PDDEBUG, "Success to update task[%lld]'s progress "
                       "with requestID[%lld]", _taskID, reqID ) ;
               goto done ;
            }
         }
      } while( cb->isInterrupted() == FALSE ) ;

      rc = SDB_APP_INTERRUPT ;
      PD_LOG( PDERROR, "Receive interrupt when update remove db business task "
              "progress to omsvc" ) ;

   done:
      if ( 0 != reqID )
      {
         pOmaMgr->unregisterTaskEvent( reqID ) ;
         reqID = 0 ;
      }
      return rc ;
   error:
      goto done ;
   }

   _omaCommand* _omaAsyncTask::createOmaCmd()
   {
      return getOmaCmdBuilder()->create( getOmaCmdName() ) ;
   }

   void _omaAsyncTask::deleteOmaCmd( _omaCommand* cmd )
   {
      getOmaCmdBuilder()->release( cmd ) ;
   }

   const CHAR* _omaAsyncTask::getOmaCmdName()
   {
      return _command.c_str() ;
   }

   INT32 _omaAsyncTask::_generateTaskPlan( BSONObj& planInfo )
   {
      INT32 rc = SDB_OK ;
      _omaCommand* planCmd = createOmaCmd() ;

      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;

      if( planCmd == NULL )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unknow omagent command, cmd=%s", getOmaCmdName() ) ;
         goto error ;
      }

      rc = _addStepVar( planCmd, OMA_STR_STEP_GENERATE_PLAN ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "Failed to add user define variable, rc=%d", rc ) ;
         goto error ;
      }

      rc = planCmd->init( _taskInfo.objdata() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init cmd, rc=%d", rc ) ;
         goto error ;
      }

      rc = planCmd->doit( planInfo ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to exec cmd, rc=%d", rc ) ;
         goto error ;
      }

      setPlanTaskStatus( OMA_TASK_STATUS_RUNNING ) ;
      updateProgressToOM() ;

   done:
      deleteOmaCmd( planCmd ) ;
      return rc ;
   error:
      _isSetErrInfo = TRUE ;
      _errno  = rc ;
      _detail = omagentGetEDUInfoSafe( EDU_INFO_ERROR ) ;
      rc = _rollbackPlan() ;
      if( rc )
      {
         PD_LOG( PDERROR, "Failed to rollback plan, rc=%d", rc ) ;
      }
      goto done ;
   }

   /*
   planInfo:
   {
      Plan: [
         [
            // Async exec
            { [a sub-task parameters]... },
            ...
         ],
         ...
      ]
   }
   */
   INT32 _omaAsyncTask::_execTaskPlan( const BSONObj& planInfo )
   {
      INT32 rc = SDB_OK ;
      _omaCommand* planCmd = NULL ;
      _pmdEDUCB* cb = pmdGetThreadEDUCB () ;
      BSONObj subTaskInfo = planInfo.getObjectField( OMA_STR_FIELD_PLAN ) ;

      cb->resetInfo( EDU_INFO_ERROR ) ;

      BSONObjIterator taskIter( subTaskInfo ) ;
      while( taskIter.more() )
      {
         BSONObj result ;
         BSONElement taskEle = taskIter.next() ;
         BSONObj oneTask = taskEle.embeddedObject() ;
         BSONObjIterator subTaskIter( oneTask ) ;

         if( cb->isInterrupted() )
         {
            goto error ;
         }

         // 1. build step argument
         _initSubTaskArg() ;
         while( subTaskIter.more() )
         {
            BSONElement subTaskEle = subTaskIter.next() ;
            BSONObj subTask = subTaskEle.embeddedObject() ;
            _appendSubTaskArg( subTask ) ;
         }

         // 2. run step
         rc = _createSubTask() ;
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to create sub task, rc=%d", rc ) ;
            goto error ;
         }

         if( _planTaskNum == 0 )
         {
            continue ;
         }

         rc = _waitSubTask() ;
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to wait sub task, rc=%d", rc ) ;
            goto error ;
         }

         // 3. exec js to check step result
         planCmd = createOmaCmd() ;
         if( planCmd == NULL )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Unknow omagent command, cmd=%s",
                    getOmaCmdName() ) ;
            goto error ;
         }

         rc = _addStepVar( planCmd, OMA_STR_STEP_CHECK_RESULT ) ;
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to add user define variable, rc=%d", rc ) ;
            goto error ;
         }

         rc = planCmd->init( _taskInfo.objdata() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to init cmd, rc=%d", rc ) ;
            goto error ;
         }

         rc = planCmd->doit( result ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to exec cmd, rc=%d", rc ) ;
            goto error ;
         }

         deleteOmaCmd( planCmd ) ;
         planCmd = NULL ;

      }

      _setTaskResultInfoStatus( OMA_TASK_STATUS_FINISH ) ;
      setPlanTaskStatus( OMA_TASK_STATUS_FINISH ) ;
      rc = updateProgressToOM() ;
      if( rc )
      {
         PD_LOG( PDERROR, "Failed to update progress to om, rc=%d", rc ) ;
         goto error ;
      }

   done:
      deleteOmaCmd( planCmd ) ;
      return rc ;
   error:
      _isSetErrInfo = TRUE ;
      _errno  = rc ;
      _detail = omagentGetEDUInfoSafe( EDU_INFO_ERROR ) ;
      rc = _rollbackPlan() ;
      if( rc )
      {
         PD_LOG( PDERROR, "Failed to rollback plan, rc=%d", rc ) ;
      }
      goto done ;
   }

   void _omaAsyncTask::_setTaskResultInfoStatus( OMA_TASK_STATUS status )
   {
      BSONObj tmpTaskInfo ;
      BSONObj resultInfo ;
      BSONObj condition = BSON( OMA_FIELD_RESULTINFO << "" ) ;
      BSONObj resultInfoCondition = BSON( OMA_FIELD_STATUS << 0 <<
                                          OMA_FIELD_STATUSDESC << "" ) ;
      BSONObjBuilder taskInfoBuilder ;
      BSONArrayBuilder resultInfoBuilder ;

      tmpTaskInfo = _taskInfo.filterFieldsUndotted( condition, FALSE ) ;

      resultInfo = _taskInfo.getObjectField( OMA_FIELD_RESULTINFO ) ;

      BSONObjIterator resultInfoIter( resultInfo ) ;
      while( resultInfoIter.more() )
      {
         BSONElement oneResultInfoEle = resultInfoIter.next() ;
         BSONObj oneResultInfo = oneResultInfoEle.embeddedObject() ;
         BSONObj tmpOneResultInfo ;
         BSONObjBuilder oneResultInfoBuilder ;

         tmpOneResultInfo = oneResultInfo.filterFieldsUndotted(
                                                  resultInfoCondition, FALSE ) ;

         oneResultInfoBuilder.appendNumber( OMA_FIELD_STATUS, status ) ;
         oneResultInfoBuilder.append( OMA_FIELD_STATUSDESC,
                                      getTaskStatusDesc( status ) ) ;
         oneResultInfoBuilder.appendElements( tmpOneResultInfo ) ;

         resultInfoBuilder.append( oneResultInfoBuilder.obj() ) ;
      }

      taskInfoBuilder.appendElements( tmpTaskInfo ) ;
      taskInfoBuilder.append( OMA_FIELD_RESULTINFO,
                                resultInfoBuilder.arr() ) ;

      _taskInfo = taskInfoBuilder.obj() ;
   }

   INT32 _omaAsyncTask::_rollbackPlan()
   {
      INT32 rc = SDB_OK ;
      BSONObj rollbackResult ;
      BSONObj resultInfo ;
      BSONElement resultInfoEle ;
      _omaCommand* planCmd = createOmaCmd() ;

      setPlanTaskStatus( OMA_TASK_STATUS_ROLLBACK ) ;
      _setTaskResultInfoStatus( OMA_TASK_STATUS_ROLLBACK ) ;
      updateProgressToOM() ;
      
      if( planCmd == NULL )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unknow omagent command, cmd=%s", getOmaCmdName() ) ;
         goto error ;
      }

      rc = _addStepVar( planCmd, OMA_STR_STEP_ROLLBACK ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "Failed to add user define variable, rc=%d", rc ) ;
         goto error ;
      }

      rc = planCmd->init( _taskInfo.objdata() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init cmd, rc=%d", rc ) ;
         goto error ;
      }

      rc = planCmd->doit( rollbackResult ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to exec cmd, rc=%d", rc ) ;
         goto error ;
      }

      resultInfoEle = rollbackResult.getField( OMA_FIELD_RESULTINFO ) ;
      if( resultInfoEle.type () != Array )
      {
         PD_LOG( PDDEBUG, "Invalid rollback result, not found %s",
                 OMA_FIELD_RESULTINFO ) ;
         goto done ;
      }

      resultInfo = resultInfoEle.embeddedObject().getOwned() ;

      {
         BSONObjIterator resultInfoIter( resultInfo ) ;
         while( resultInfoIter.more() )
         {
            BSONElement oneResultInfoEle = resultInfoIter.next() ;
            BSONObj oneResultInfo = oneResultInfoEle.embeddedObject() ;

            rc = updateTaskInfo( planCmd, oneResultInfo ) ;
            if( rc )
            {
               PD_LOG( PDERROR, "Failed to update task info, rc=%d", rc ) ;
               goto error ;
            }
         }
      }

   done:
      _setTaskResultInfoStatus( OMA_TASK_STATUS_FINISH ) ;
      setPlanTaskStatus( OMA_TASK_STATUS_FINISH ) ;
      updateProgressToOM() ;
      return rc ;
   error:
      _isSetErrInfo = TRUE ;
      _errno  = rc ;
      _detail = omagentGetEDUInfoSafe( EDU_INFO_ERROR ) ;
      goto done ;
   }

   INT32 _omaAsyncTask::_createSubTask()
   {
      INT32 rc = SDB_OK ;
      INT32 startTaskNum = 0 ;
      INT32 subTaskNum   = _planTaskArgList.size() ;

      _planTaskNum = 0 ;
      if( _planTaskArgList.empty() )
      {
         PD_LOG( PDWARNING, "There is no need to perform subtasks" ) ;
         goto done ;
      }

      subTaskNum = subTaskNum > MAX_THREAD_NUM ? MAX_THREAD_NUM : subTaskNum ;

      for( INT32 i = 0; i < subTaskNum; ++i )
      {
         omaTaskPtr taskPrt ;
         rc = startOmagentJob( OMA_TASK_ASYNC_SUB, _taskID, BSONObj(),
                               taskPrt, (void*)this ) ;
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to run sub task with the type[%d], rc=%d",
                    OMA_TASK_ASYNC_SUB, rc ) ;
            goto error ;
         }
         ++startTaskNum ;
      }

      _planTaskNum = startTaskNum ;

   done:
      return rc ;
   error:
      _setTaskResultInfoStatus( OMA_TASK_STATUS_FAIL ) ;
      setPlanTaskStatus( OMA_TASK_STATUS_FAIL ) ;
      updateProgressToOM() ;
      if( startTaskNum > 0 )
      {
         rc = _waitSubTask() ;
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to wait sub task, rc=%d", rc ) ;
         }
      }
      goto done ;
   }

   INT32 _omaAsyncTask::_waitSubTask()
   {
      INT32 rc = SDB_OK ;
      _pmdEDUCB* cb = pmdGetThreadEDUCB () ;

      if( _planTaskNum == 0 )
      {
         PD_LOG( PDWARNING, "No task, no waiting" ) ;
         goto done ;
      }

      PD_LOG( PDDEBUG, "wait %d sub task", _planTaskNum ) ;

      while( cb->isInterrupted() == FALSE && _planTaskNum > 0 )
      {
         if( _planEvent.wait( OMA_WAIT_SUB_TASK_NOTIFY_TIMEOUT ) )
         {
            //timeout
            continue ;
         }
         _planEvent.reset() ;
      }

      if( _planTaskNum > 0 )
      {
         rc = SDB_APP_INTERRUPT ;
         PD_LOG( PDERROR, "Receive interrupt when running install db "
                          "business sub task" ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "All the sub tasks had finished" ) ;

   done:
      return rc ;
   error:
      goto done ; 
   }

   void _omaAsyncTask::_initSubTaskArg()
   {
      _planTaskIndex = 0 ;
      _planTaskArgList.clear() ;
   }

   void _omaAsyncTask::_appendSubTaskArg( BSONObj& subTaskArg )
   {
      ossScopedLock lock( &_planLatch, EXCLUSIVE ) ;

      _planTaskArgList.push_back( subTaskArg ) ;
   }

   BOOLEAN _omaAsyncTask::getSubTaskArg( UINT32& planTaskIndex,
                                         BSONObj& subTaskArg )
   {
      ossScopedLock lock( &_planLatch, EXCLUSIVE ) ;
      BOOLEAN result = TRUE ;

      if( _planTaskIndex == _planTaskArgList.size() )
      {
         result = FALSE ;
         goto done ; 
      }
      planTaskIndex = _planTaskIndex ;
      subTaskArg = _planTaskArgList[ _planTaskIndex ] ;
      ++_planTaskIndex ;

   done:
      return result ;
   }

   void _omaAsyncTask::setPlanTaskStatus( OMA_TASK_STATUS status )
   {
      ossScopedLock lock( &_planLatch, EXCLUSIVE ) ;

      _taskStatus = status ;
   }

   OMA_TASK_STATUS _omaAsyncTask::getPlanTaskStatus()
   {
      ossScopedLock lock( &_planLatch, EXCLUSIVE ) ;

      return _taskStatus ;
   }


   _omaAsyncSubTask::_omaAsyncSubTask( INT64 taskID ) : _omaTask( taskID )
   {
      _taskID = taskID ;
      _taskName = OMA_CMD_ASYNC_SUB_TASK ;
   }

   _omaAsyncSubTask::~_omaAsyncSubTask()
   {
   }

   INT32 _omaAsyncSubTask::init( const BSONObj& info, void* ptr )
   {
      SDB_ASSERT( ptr != NULL, "omagent task can't not be null" ) ;

      _task = (_omaAsyncTask*)ptr ;
      _taskInfo = info.copy() ;

      return SDB_OK ;
   }

   INT32 _omaAsyncSubTask::doit()
   {
      INT32 rc     = SDB_OK ;
      UINT32 index = 0 ;
      _omaCommand* cmd = NULL ;
      _pmdEDUCB* cb    = pmdGetThreadEDUCB() ;
      BSONObj argument ;
      BSONObj result ;
      //BSONObj newResult ;

      while( _task->getSubTaskArg( index, argument ) )
      {
         if( cb->isInterrupted() )
         {
            goto error ;
         }

         cmd = _task->createOmaCmd() ;
         if( cmd == NULL )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to create omagent command, command=%s",
                    _task->getOmaCmdName() ) ;
            goto error ;
         }

         rc = _task->setTaskRunning( cmd, argument ) ;
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to set task status, rc=%d", rc ) ;
            rc = SDB_OK ;
         }

         _task->updateProgressToOM() ;

         rc = _addStepVar( cmd, OMA_STR_STEP_DOIT ) ;
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to add user define variable, rc=%d", rc ) ;
            goto error ;
         }

         rc = cmd->init( argument.objdata() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to init omagent command, rc=%d", rc ) ;
            goto error ;
         }

         rc = cmd->doit( result ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to run omagent command, rc=%d", rc ) ;
            goto error ;
         }

         rc = _task->updateTaskInfo( cmd, result ) ;
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to update task info, rc=%d", rc ) ;
            goto error ;
         }

         _task->updateProgressToOM() ;

         _task->deleteOmaCmd( cmd ) ;
         cmd = NULL ;

      }

   done:
      _task->deleteOmaCmd( cmd ) ;
      _task->notifyUpdateProgress() ;
      PD_LOG( PDDEBUG, "finish async sub task" ) ;
      return rc ;
   error:
      _task->setPlanTaskStatus( OMA_TASK_STATUS_FAIL ) ;
      _task->updateProgressToOM() ;
      goto done ;
   }


}

