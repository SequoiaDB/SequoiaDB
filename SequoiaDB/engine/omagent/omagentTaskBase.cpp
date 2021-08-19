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

   Source File Name = omagentTaskBase.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#include "ossTypes.h"
#include "pmdDef.hpp"
#include "pmdEDU.hpp"
#include "omagentUtil.hpp"
#include "omagentTaskBase.hpp"
#include "omagentJob.hpp"
#include "omagentBackgroundCmd.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   const CHAR* getTaskStatusDesc( OMA_TASK_STATUS status )
   {
      switch ( status )
      {
      case OMA_TASK_STATUS_INIT :
         return OMA_TASK_STATUS_DESC_INIT ;
      case OMA_TASK_STATUS_RUNNING :
         return OMA_TASK_STATUS_DESC_RUNNING ;
      case OMA_TASK_STATUS_ROLLBACK :
         return OMA_TASK_STATUS_DESC_ROLLBACK ;
      case OMA_TASK_STATUS_CANCEL :
         return OMA_TASK_STATUS_DESC_CANCEL ;
      case OMA_TASK_STATUS_FINISH :
      case OMA_TASK_STATUS_FAIL :
         return OMA_TASK_STATUS_DESC_FINISH ;
      default:
         return OMA_TASK_STATUS_DESC_UNKNOWN ;
      }
   }

   /*
      omagent task
   */
   OMA_TASK_TYPE _omaTask::getTaskType()
   { 
      return _taskType ;
   }
   
   INT64 _omaTask::getTaskID()
   { 
      return _taskID ;
   }
   
   const CHAR* _omaTask::getTaskName()
   { 
      return _taskName.c_str() ;
   }
   
   OMA_TASK_STATUS _omaTask::getTaskStatus ()
   {
      ossScopedLock lock ( &_latch, EXCLUSIVE ) ; 
      return _taskStatus ;
   }
 
   void _omaTask::setTaskStatus( OMA_TASK_STATUS status )
   {
      ossScopedLock lock ( &_latch, EXCLUSIVE ) ;   
      _taskStatus = status ;
   }

   INT32 _omaTask::setSubTaskStatus( string &name, OMA_TASK_STATUS status )
   {
      INT32 rc = SDB_OK ;
      ossScopedLock lock ( &_latch, EXCLUSIVE ) ;

      if ( name.empty() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Invalid sub task name" ) ;
         goto error ;
      }
      PD_LOG ( PDDEBUG, "Set Sub task[%s]'s status to be [%d]",
               name.c_str(), status ) ;
      _subTaskStatus[name] = status ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omaTask::getSubTaskStatus( string &name, OMA_TASK_STATUS &status )
   {
      INT32 rc =SDB_OK ;
      map< string, OMA_TASK_STATUS >::iterator it ;

      it = _subTaskStatus.find( name ) ;
      if ( _subTaskStatus.end() != it )
      {
         status = it->second ;
      }
      else
      {
         PD_LOG ( PDERROR, "Failed to get sub task's status, no such "
                  "task named %s", name.c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omaTask::getSubTaskSerialNum()
   {
      ossScopedLock lock ( &_latch, EXCLUSIVE ) ;
      return _subTaskSerialNum++ ;
   }

   void _omaTask::setJobInfo( EDUID eduID )
   {
      _eduID = eduID ;
   }
   
   EDUID _omaTask::getJobInfo()
   {
      return _eduID ;
   }

   INT32 _omaTask::initJsEnv()
   {
      INT32 rc     = SDB_OK ;
      INT32 tmpRc  = SDB_OK ;
      INT32 errNum = 0 ;
      const CHAR *pDetail = NULL ;
      BSONObj obj ;
      BSONObj retObj ;
      _omaInitEnv runCmd( _taskID, obj ) ;

      // 1. run command
      rc = runCmd.init( NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Failed to init for running js script, "
                 "rc = %d", rc ) ;
         goto error ;
      }
      rc = runCmd.doit( retObj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Failed to init for running js script, "
                 "rc = %d", rc ) ;
         goto error ;
      }
      // 2. check
      // extract "errno"
      rc = omaGetIntElement ( retObj, OMA_FIELD_ERRNO, errNum ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get errno from js after initializing "
                 "environment for execting js script, rc = %d", rc ) ;
         goto error ;
      }
      // to see whether execute js successfully or not
      if ( SDB_OK != errNum )
      {
         rc = errNum ;
         // get error detail
         tmpRc = omaGetStringElement ( retObj, OMA_FIELD_DETAIL, &pDetail ) ;
         if ( SDB_OK != tmpRc )
         {
            PD_LOG( PDERROR, "Failed to get error detail from js after "
                    "environment for execting js script, rc = %d", rc ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to init environment for execting js"
                    "script, rc = %d, detail = %s", rc, pDetail ) ;
         }
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      omagent manager
   */
   _omaTaskMgr::_omaTaskMgr ( INT64 taskID )
   {
      _taskID = taskID ;
   }

   _omaTaskMgr::~_omaTaskMgr ()
   {
      map<INT64, _omaTask*>::iterator it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         SDB_OSS_DEL it->second ;
         ++it ;
      }
      _taskMap.clear() ;
   }

   INT64 _omaTaskMgr::getTaskID ()
   {
      INT64 id = OMA_INVALID_TASKID ;
      std::map<INT64, _omaTask*>::iterator it ;

      ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;
      while ( TRUE )
      {
         id = ++_taskID ;
         it = _taskMap.find( id ) ;
         if ( it == _taskMap.end() )
         {
            break ;
         }
      }
      
      return id ;
   }

   INT32 _omaTaskMgr::addTask ( _omaTask * pTask, INT64 taskID )
   {
      INT32 rc = SDB_OK ;
      _omaTask *indexTask = NULL ;

      if ( OMA_INVALID_TASKID == taskID )
      {
         taskID = pTask->getTaskID() ;
      }

      ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;

      std::map<INT64, _omaTask*>::iterator it ;
      it = _taskMap.find( taskID ) ;
      if ( it != _taskMap.end() )
      {
           indexTask = it->second ;
           PD_LOG ( PDWARNING, "Exist task[%lld,%s] mutex with new task[%lld,%s]",
                    indexTask->getTaskID(), indexTask->getTaskName(),
                    pTask->getTaskID(), pTask->getTaskName() ) ;
           rc = SDB_CLS_MUTEX_TASK_EXIST ;
           goto error ;
      }
      // add to map
      _taskMap[ taskID ] = pTask ;
   done:
      return rc ;
   error:
      SDB_OSS_DEL pTask ;
      goto done ;
   }

   INT32 _omaTaskMgr::removeTask ( INT64 taskID )
   {
      ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;
      std::map<INT64, _omaTask*>::iterator it = _taskMap.find ( taskID ) ;
      if ( it != _taskMap.end() )
      {
         SDB_OSS_DEL it->second ;
         _taskMap.erase ( it ) ;
      }
      return SDB_OK ;
   }

   INT32 _omaTaskMgr::removeTask ( _omaTask * pTask )
   {
      INT32 rc = SDB_OK ;
      rc = removeTask ( pTask->getTaskID () ) ;
      return rc ;
   }

   INT32 _omaTaskMgr::removeTask ( const CHAR *pTaskName )
   {
      INT32 rc = SDB_OK ;
      std::map<INT64, _omaTask*>::iterator it = _taskMap.begin() ;
      PD_LOG( PDDEBUG, "There are [%d] task kept in task manager, "
              "the removing task is[%s]", _taskMap.size(), pTaskName ) ;
      for ( ; it != _taskMap.end(); it++ )
      {
         _omaTask *pTask = it->second ;
         const CHAR *name = pTask->getTaskName() ;
         PD_LOG ( PDDEBUG, "The task is [%s]", name ) ;
         if ( 0 == ossStrncmp( name, pTaskName, ossStrlen(pTaskName) ) )
         {
            rc = removeTask( pTask ) ;
            // when remove old task, must stop iterate
            break ;
         }
      }
      return rc ;
   }

   _omaTask* _omaTaskMgr::findTask ( INT64 taskID )
   {
      ossScopedLock lock ( &_taskLatch, SHARED ) ;
      std::map<INT64, _omaTask*>::iterator it = _taskMap.find ( taskID ) ;
      if ( it != _taskMap.end() )
      {
         return it->second ;
      }
      return NULL ;
   }

   // get omagent task manager
   _omaTaskMgr* getTaskMgr()
   {
      static _omaTaskMgr taskMgr ;
      return &taskMgr ;
   }
   
} // namespace engine
