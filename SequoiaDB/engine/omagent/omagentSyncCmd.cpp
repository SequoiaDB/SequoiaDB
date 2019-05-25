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

   Source File Name = omagentCommand.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#include "omagentSyncCmd.hpp"
#include "omagentUtil.hpp"
#include "omagentHelper.hpp"
#include "ossProc.hpp"
#include "utilPath.hpp"
#include "omagentJob.hpp"
#include "omagentMgr.hpp"

using namespace bson ;

namespace engine
{
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaScanHost )
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaPreCheckHost )
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaCheckHost )
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaPostCheckHost )
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaUpdateHostsInfo )
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaQueryHostStatus )
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaHandleTaskNotify )
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaHandleInterruptTask )
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaHandleSsqlGetMore )
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaSyncBuzConfigure )
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaCreateRelationship )
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaRemoveRelationship )

   /******************************* scan host *********************************/
   /*
      _omaScanHost
   */
   _omaScanHost::_omaScanHost()
   {
   }

   _omaScanHost::~_omaScanHost()
   {
   }

   INT32 _omaScanHost::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus( pInstallInfo ) ;
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Scan host passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_SCAN_HOST, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SCAN_HOST, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   /******************************* pre-check host ****************************/
   /*
      _omaPreCheckHost
   */
   _omaPreCheckHost::_omaPreCheckHost ()
   {
   }

   _omaPreCheckHost::~_omaPreCheckHost ()
   {
   }

   INT32 _omaPreCheckHost::init ( const CHAR *pInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus( pInfo ) ;

      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;
      PD_LOG ( PDDEBUG, "Pre-check host passes argument: %s",
               _jsFileArgs.c_str() ) ;
      rc = addJsFile( FILE_CHECK_HOST_INIT, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                  FILE_CHECK_HOST_INIT, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /******************************* check host ********************************/
   /*
      _omaCheckHost
   */
   _omaCheckHost::_omaCheckHost ()
   {
   }

   _omaCheckHost::~_omaCheckHost ()
   {
   }

   INT32 _omaCheckHost::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;

      try
      {
         stringstream ss ;
         BSONObj bus( pInstallInfo ) ;

         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Check host info passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_CHECK_HOST_ITEM ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_CHECK_HOST_ITEM, rc ) ;
            goto error ;
         }
         rc = addJsFile( FILE_CHECK_HOST, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_CHECK_HOST, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   /******************************* post-check host ***************************/
   /*
      _omaPostCheckHost
   */
   _omaPostCheckHost::_omaPostCheckHost ()
   {
   }

   _omaPostCheckHost::~_omaPostCheckHost ()
   {
   }

   INT32 _omaPostCheckHost::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus( pInstallInfo ) ;

         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Post-check host passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_CHECK_HOST_FINAL, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_CHECK_HOST_FINAL, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   /******************************* remove host *******************************/
   /*
      _omaRemoveHost
   */
/*
   _omaRemoveHost::_omaRemoveHost ()
   {
   }

   _omaRemoveHost::~_omaRemoveHost ()
   {
   }

   INT32 _omaRemoveHost::init( const CHAR *pInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj bus( pInfo ) ;

         ossSnprintf( _jsFileArgs, JS_ARG_LEN, "var %s = %s; ",
                      JS_ARG_BUS, bus.toString(FALSE, TRUE).c_str() ) ;
         PD_LOG ( PDDEBUG, "Remove hosts passes argument: %s",
                  _jsFileArgs ) ;
         rc = addJsFile( FILE_REMOVE_HOST, _jsFileArgs ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_REMOVE_HOST, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }
*/
   /*************************** update hosts table info ***********************/
   /*
      _omaUpdateHostsInfo
   */
   _omaUpdateHostsInfo::_omaUpdateHostsInfo ()
   {
   }

   _omaUpdateHostsInfo::~_omaUpdateHostsInfo ()
   {
   }

   INT32 _omaUpdateHostsInfo::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus( pInstallInfo ) ;
      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;
      PD_LOG ( PDDEBUG, "Update hosts info passes argument: %s",
               _jsFileArgs.c_str() ) ;
      rc = addJsFile ( FILE_UPDATE_HOSTS_INFO, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to add js file[%s]", FILE_UPDATE_HOSTS_INFO ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*************************** query host status **************************/
   /*
      _omaQueryHostStatus
   */
   _omaQueryHostStatus::_omaQueryHostStatus()
   {
   }

   _omaQueryHostStatus::~_omaQueryHostStatus()
   {
   }

   INT32 _omaQueryHostStatus::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus( pInstallInfo ) ;

         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "_omaQueryHostStatus passes argument: %s",
                  _jsFileArgs.c_str() ) ;

         rc = addJsFile( FILE_QUERY_HOSTSTATUS_ITEM ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_QUERY_HOSTSTATUS_ITEM, rc ) ;
            goto error ;
         }

         rc = addJsFile( FILE_QUERY_HOSTSTATUS, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_QUERY_HOSTSTATUS, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson, exception is: %s",
                      e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error :
      goto done ;
   }


   /*************************** handle task notify ****************************/
   /*
      _omaHandleTaskNotify
   */
   _omaHandleTaskNotify::_omaHandleTaskNotify()
   {
   }

   _omaHandleTaskNotify::~_omaHandleTaskNotify()
   {
   }

   INT32 _omaHandleTaskNotify::init ( const CHAR *pInstallInfo )
   {
      INT32 rc      = SDB_OK ;
      UINT64 taskID = 0 ;
      BSONObj obj ;
      BSONElement ele ;
      try
      {
         obj = BSONObj( pInstallInfo ).copy() ;
         ele = obj.getField( OMA_FIELD_TASKID ) ;
         if ( NumberInt != ele.type() && NumberLong != ele.type() )
         {
            PD_LOG_MSG ( PDERROR, "Receive invalid task id from omsvc" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         taskID = ele.numberLong() ;
         _taskIDObj = BSON( OMA_FIELD_TASKID << (INT64)taskID ) ;
         
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error :
      goto done ;
   }

   INT32 _omaHandleTaskNotify::doit ( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      
      rc = sdbGetOMAgentMgr()->startTaskCheckImmediately( _taskIDObj ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to start task check, rc = %d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*************************** handle interrupt task ****************************/
   /*
      _omaHandleInterruptTask
   */
   _omaHandleInterruptTask::_omaHandleInterruptTask()
   {
   }

   _omaHandleInterruptTask::~_omaHandleInterruptTask()
   {
   }

   INT32 _omaHandleInterruptTask::init ( const CHAR *pInterruptInfo )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONElement ele ;
      try
      {
         obj = BSONObj( pInterruptInfo ).copy() ;
         ele = obj.getField( OMA_FIELD_TASKID ) ;
         if ( NumberInt != ele.type() && NumberLong != ele.type() )
         {
            PD_LOG_MSG ( PDERROR, "Receive invalid task id from omsvc" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _taskID = ele.numberLong() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error :
      goto done ;
   }

   INT32 _omaHandleInterruptTask::doit ( BSONObj &retObj )
   {
      INT32 rc        = SDB_OK ;
      _omaTask *pTask = NULL ;
      omaTaskPtr taskPtr ;
      string detail ;
      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;
      rc = sdbGetOMAgentMgr()->getTaskInfo( _taskID, taskPtr );
      if ( SDB_OK != rc )
      {
         rc = SDB_OM_TASK_NOT_EXIST ;
         PD_LOG_MSG( PDERROR, "task is not exist:task="OSS_LL_PRINT_FORMAT
                     ",rc=%d", _taskID, rc ) ;
         goto error ;
      }

      pTask = taskPtr.get() ;

      if ( OMA_TASK_SSQL_EXEC != pTask->getTaskType() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "task is not ssql_exec, can't interrupt:type=%d,"
                     "taskID="OSS_LL_PRINT_FORMAT, pTask->getTaskType(), 
                     _taskID ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "interrupt task="OSS_LL_PRINT_FORMAT, _taskID ) ;
      rc = pmdGetKRCB()->getEDUMgr()->postEDUPost( pTask->getJobInfo(), 
                                                   PMD_EDU_EVENT_TERM ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "send term message to task failed:task="
                     OSS_LL_PRINT_FORMAT",rc=%d", _taskID, rc ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error:
      detail = pmdGetThreadEDUCB()->getInfo( EDU_INFO_ERROR ) ;
      retObj = BSON( OMA_FIELD_DETAIL << detail ) ;
      goto done ;
   }

   /***************************** handle ssql get more **************************/
   /*
      _omaHandleSsqlGetMore
   */
   _omaHandleSsqlGetMore::_omaHandleSsqlGetMore()
   {
   }

   _omaHandleSsqlGetMore::~_omaHandleSsqlGetMore()
   {
   }

   INT32 _omaHandleSsqlGetMore::init ( const CHAR *pInterruptInfo )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONElement ele ;
      try
      {
         obj = BSONObj( pInterruptInfo ).copy() ;
         ele = obj.getField( OMA_FIELD_TASKID ) ;
         if ( NumberInt != ele.type() && NumberLong != ele.type() )
         {
            PD_LOG_MSG( PDERROR, "Receive invalid task id from omsvc" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _taskID = ele.numberLong() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Failed to build bson, exception is: %s",
                     e.what() ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error :
      goto done ;
   }

   INT32 _omaHandleSsqlGetMore::doit ( BSONObj &retObj )
   {
      INT32 rc        = SDB_OK ;
      _omaTask *pTask = NULL ;
      omaTaskPtr taskPtr ;
      _omaSsqlExecTask *pSsqlExecTask = NULL ;
      list<ssqlRowData_t> data ;
      list<ssqlRowData_t>::iterator iter ;
      BSONArrayBuilder arrayBuilder ;
      BOOLEAN isFinish = FALSE ;
      string detail ;
      INT32 status = OMA_TASK_STATUS_RUNNING ;
      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;
      rc = sdbGetOMAgentMgr()->getTaskInfo( _taskID, taskPtr );
      if ( SDB_OK != rc )
      {
         rc = SDB_OM_TASK_NOT_EXIST ;
         PD_LOG_MSG( PDERROR, "task is not exist:task="OSS_LL_PRINT_FORMAT
                     ",rc=%d", _taskID, rc ) ;
         goto error ;
      }

      pTask = taskPtr.get() ;

      if ( OMA_TASK_SSQL_EXEC != pTask->getTaskType() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "task is not ssql_exec, can't SsqlGetMore:"
                     "type=%d,taskID="OSS_LL_PRINT_FORMAT, 
                     pTask->getTaskType(), _taskID ) ;
         goto error ;
      }

      pSsqlExecTask = ( _omaSsqlExecTask *)pTask ;
      rc = pSsqlExecTask->getSqlData( data, isFinish ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "get sql data failed:rc=%d,task="
                     OSS_LL_PRINT_FORMAT, rc, _taskID ) ;
         goto error ;
      }
      
      for ( iter = data.begin() ; iter != data.end() ; iter++ )
      {
         BSONObj oneRow = BSON( OMA_FIELD_ROWNUM << (long long )iter->rowNum 
                                << OMA_FIELD_ROWVALUE << iter->rowData ) ;
         arrayBuilder.append( oneRow ) ;
      }

      if ( isFinish )
      {
         status = OMA_TASK_STATUS_FINISH ;
      }

      retObj = BSON( OMA_FIELD_STATUS << status << 
                     OMA_FIELD_RESULTINFO << arrayBuilder.arr() ) ;

   done:
      return rc ;
   error:
      detail = pmdGetThreadEDUCB()->getInfo( EDU_INFO_ERROR ) ;
      retObj = BSON( OMA_FIELD_DETAIL << detail ) ;
      goto done ;
   }

   /***********************  sync business configure  *************************/
   /*
      _omaSyncBuzConfigure
   */
   _omaSyncBuzConfigure::_omaSyncBuzConfigure()
   {
   }

   _omaSyncBuzConfigure::~_omaSyncBuzConfigure()
   {
   }

   INT32 _omaSyncBuzConfigure::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus( pInstallInfo ) ;

         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Scan host passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_SYNC_BUSINESS_CONF, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SCAN_HOST, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   /************************** create relationship ************************/
   /*
      _omaCreateRelationship
   */
   _omaCreateRelationship::_omaCreateRelationship()
   {
   }

   _omaCreateRelationship::~_omaCreateRelationship()
   {
   }

   INT32 _omaCreateRelationship::init( const CHAR *pInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus( pInfo ) ;

         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Scan host passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_CREATE_RELATIONSHIP, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SCAN_HOST, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   /************************** remove relationship ************************/
   /*
      _omaRemoveRelationship
   */
   _omaRemoveRelationship::_omaRemoveRelationship()
   {
   }

   _omaRemoveRelationship::~_omaRemoveRelationship()
   {
   }

   INT32 _omaRemoveRelationship::init( const CHAR *pInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus( pInfo ) ;

         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Scan host passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_REMOVE_RELATIONSHIP, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SCAN_HOST, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

} // namespace engine

