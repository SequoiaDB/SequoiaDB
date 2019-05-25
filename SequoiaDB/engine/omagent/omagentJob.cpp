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

   Source File Name = omagentJob.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/
#include "omagentUtil.hpp"
#include "omagentJob.hpp"
#include "omagentBackgroundCmd.hpp"
#include "omagentAsyncTask.hpp"
#include "pmdEDU.hpp"

namespace engine
{
   /*
      omagent job
   */
   _omagentJob::_omagentJob ( omaTaskPtr taskPtr, const BSONObj &info, void *ptr )
   {
      _taskPtr = taskPtr ;
      _info    = info.copy() ;

      _pointer = ptr ;

      _omaTask *pTask = _taskPtr.get() ;
      if ( pTask )
         _jobName = _jobName + "Omagent job for task[" +
                    pTask->getTaskName() + "]" ;
   }

   _omagentJob::~_omagentJob()
   {
   }

   RTN_JOB_TYPE _omagentJob::type () const
   {
      return RTN_JOB_OMAGENT ;
   }

   const CHAR* _omagentJob::name () const
   {
      return _jobName.c_str() ;
   }

   BOOLEAN _omagentJob::muteXOn ( const _rtnBaseJob *pOther )
   {
      return FALSE ;
   }

   INT32 _omagentJob::doit()
   {
      INT32 rc = SDB_OK ;
      _omaTask *pTask = _taskPtr.get() ;

      pmdSetEDUHook( (PMD_ON_EDU_EXIT_FUNC)sdbHookFuncOnThreadExit ) ;

      if ( NULL == pTask )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid task poiter" ) ;
         goto error ;
      }
      rc = pTask->init( _info, _pointer ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init in job[%s] for running task[%s], "
                 "rc = %d", _jobName.c_str(), pTask->getTaskName(), rc ) ;
         goto error ;
      }
      rc = pTask->doit() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to do it in job[%s] for running task[%s], "
                 "rc = %d", _jobName.c_str(), pTask->getTaskName(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }





   INT32 startOmagentJob ( OMA_TASK_TYPE taskType, INT64 taskID,
                           const BSONObj &info, omaTaskPtr &taskPtr, void *ptr )
   {
      INT32 rc               = SDB_OK ;
      EDUID eduID            = PMD_INVALID_EDUID ;
      BOOLEAN returnResult   = FALSE ;
      _omagentJob *pJob      = NULL ;
      _omaTask *pTask        = NULL ;

      pTask = getTaskByType( taskType, taskID ) ;
      if ( NULL == pTask )
      {
         PD_LOG( PDERROR, "Unkown task type" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      {
         omaTaskPtr myTaskPtr( pTask ) ;
         pJob = SDB_OSS_NEW _omagentJob( myTaskPtr, info, ptr ) ;
         if ( !pJob )
         {
            PD_LOG ( PDERROR, "Failed to alloc memory for running task "
                     "with the type[%d]", taskType ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_NONE, &eduID,
                                        returnResult ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to start task with the type[%d], rc = %d",
                     taskType, rc ) ;
            goto done ;
         }

         pTask->setJobInfo( eduID ) ;

         taskPtr = myTaskPtr ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   string getCmdByType( OMA_TASK_TYPE taskType )
   {
      string command ;
      switch( taskType )
      {
      case OMA_TASK_ADD_BUS:
         command = OMA_CMD_ADD_BUSINESS ;
         break ;
      case OMA_TASK_REMOVE_BUS:
         command = OMA_CMD_REMOVE_BUSINESS ;
         break ;
      case OMA_TASK_EXTEND_DB:
         command = OMA_CMD_EXTEND_SEQUOIADB ;
         break ;
      case OMA_TASK_SHRINK_BUSINESS:
         command = OMA_CMD_SHRINK_BUSINESS ;
         break ;
      case OMA_TASK_DEPLOY_PACKAGE:
         command = OMA_CMD_DEOLOY_PACKAGE ;
         break ;
      default:
         command = "" ;
         break ;
      }
      return command ;
   }

   _omaTask* getTaskByType( OMA_TASK_TYPE taskType, INT64 taskID )
   {
      _omaTask *pTask = NULL ;
      
      switch ( taskType )
      {
         case OMA_TASK_ADD_HOST :
            pTask = SDB_OSS_NEW _omaAddHostTask( taskID ) ;
            break ;
         case OMA_TASK_ADD_HOST_SUB :
            pTask = SDB_OSS_NEW _omaAddHostSubTask( taskID ) ;
            break ;
         case OMA_TASK_REMOVE_HOST :
            pTask = SDB_OSS_NEW _omaRemoveHostTask( taskID ) ;
            break ;
         case OMA_TASK_INSTALL_DB :
            pTask = SDB_OSS_NEW _omaInstDBBusTask( taskID ) ;
            break ;
         case OMA_TASK_INSTALL_DB_SUB :
            pTask = SDB_OSS_NEW _omaInstDBBusSubTask( taskID ) ;
            break ;
         case OMA_TASK_REMOVE_DB :
            pTask = SDB_OSS_NEW _omaRemoveDBBusTask( taskID ) ;
            break ;
         case OMA_TASK_INSTALL_ZN :
            pTask = SDB_OSS_NEW _omaInstZNBusTask( taskID ) ;
            break ;
         case OMA_TASK_INSTALL_ZN_SUB :
            pTask = SDB_OSS_NEW _omaInstZNBusSubTask( taskID ) ;
            break ;
         case OMA_TASK_REMOVE_ZN :
            pTask = SDB_OSS_NEW _omaRemoveZNBusTask( taskID ) ;
            break ;
         case OMA_TASK_INSTALL_SSQL_OLAP :
            pTask = SDB_OSS_NEW _omaInstallSsqlOlapBusTask( taskID ) ;
            break ;
         case OMA_TASK_REMOVE_SSQL_OLAP :
            pTask = SDB_OSS_NEW _omaRemoveSsqlOlapBusTask( taskID ) ;
            break ;
         case OMA_TASK_INSTALL_SSQL_OLAP_SUB :
            pTask = SDB_OSS_NEW _omaInstallSsqlOlapBusSubTask( taskID ) ;
            break ;
         case OMA_TASK_SSQL_EXEC :
            pTask = SDB_OSS_NEW _omaSsqlExecTask( taskID ) ;
            break ;
         case OMA_TASK_ASYNC_SUB:
            pTask = SDB_OSS_NEW _omaAsyncSubTask( taskID ) ;
            break ;
         case OMA_TASK_EXTEND_DB:
         case OMA_TASK_ADD_BUS:
         case OMA_TASK_REMOVE_BUS:
         case OMA_TASK_SHRINK_BUSINESS:
         case OMA_TASK_DEPLOY_PACKAGE:
            {
               string command = getCmdByType( taskType ) ;

               pTask = SDB_OSS_NEW _omaAsyncTask( taskID, command.c_str() ) ;
            }
            break ;
         default :
            PD_LOG_MSG( PDERROR, "Unknow task type[%d]", taskType ) ;
            break ;
      }
      if ( NULL == pTask )
      {
         PD_LOG_MSG( PDERROR, "Failed to malloc for task with the type[%d]",
                     taskType ) ;
      }
      return pTask ;
   }

   

}
