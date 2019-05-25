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

   Source File Name = omagentSubTask.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#include "ossTypes.h"
#include "omagentUtil.hpp"
#include "pmdDef.hpp"
#include "pmdEDU.hpp"
#include "omagentSubTask.hpp"
#include "omagentBackgroundCmd.hpp"


namespace engine
{
   

   /*
      add host sub task
   */
   _omaAddHostSubTask::_omaAddHostSubTask( INT64 taskID )
   : _omaTask( taskID )
   {
      _taskType = OMA_TASK_ADD_HOST_SUB ;
      _taskName = OMA_TASK_NAME_ADD_HOST_SUB ;
   }

   _omaAddHostSubTask::~_omaAddHostSubTask()
   {
   }


   INT32 _omaAddHostSubTask::init( const BSONObj &info, void *ptr )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      _pTask = (_omaAddHostTask *)ptr ;
      if ( NULL == _pTask )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "No add host task's info for "
                     "add host sub task" ) ;
         goto error ;
      }
      ss << _taskName << "[" << _pTask->getSubTaskSerialNum() << "]" ;
      _taskName = ss.str() ;
      
      done:
         return rc ;
      error:
         goto done ;
   }

   INT32 _omaAddHostSubTask::doit()
   {
      INT32 rc      = SDB_OK ;
      INT32 tmpRc   = SDB_OK ;

      _pTask->setSubTaskStatus( _taskName, OMA_TASK_STATUS_RUNNING ) ;
      
      while( TRUE )
      {
         AddHostInfo *pInfo           = NULL ;
         AddHostResultInfo resultInfo = { "", "", OMA_TASK_STATUS_RUNNING,
                                          OMA_TASK_STATUS_DESC_RUNNING,
                                          SDB_OK, "" } ;
         CHAR flow[OMA_BUFF_SIZE + 1] = { 0 } ;
         const CHAR *pDetail          = NULL ;
         const CHAR *pIP              = NULL ;
         const CHAR *pHostName        = NULL ;
         INT32 errNum                 = 0 ;
         stringstream ss ;
         string version ;
         BSONObj retObj ;

         if ( TRUE == pmdGetThreadEDUCB()->isInterrupted() )
         {
            PD_LOG( PDEVENT, "Program has been interrupted, stop task[%s]",
                    _taskName.c_str() ) ;
            goto done ;
         }

         pInfo = _pTask->getAddHostItem() ;
         if ( NULL == pInfo )
         {
            PD_LOG( PDEVENT, "No hosts need to add now, sub task[%s] exits",
                    _taskName.c_str() ) ;
            goto done ;
         }

         pIP                  = pInfo->_item._ip.c_str() ;
         pHostName            = pInfo->_item._hostName.c_str() ;
         resultInfo._ip       = pIP ;
         resultInfo._hostName = pHostName ;

         ossSnprintf( flow, OMA_BUFF_SIZE, "Adding host[%s]", pIP ) ;
         resultInfo._flow.push_back( flow ) ;
         tmpRc = _pTask->updateProgressToTask( pInfo->_serialNum, resultInfo ) ;
         if ( tmpRc )
         {
            PD_LOG( PDWARNING, "Failed to update add host[%s]'s progress, "
                    "rc = %d", pIP, tmpRc ) ;
         }

         _omaAddHost runCmd( *pInfo ) ;
         rc = runCmd.init( NULL ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to init for adding "
                    "host[%s], rc = %d", pIP, rc ) ;
            pDetail = pmdGetThreadEDUCB()->getInfo( EDU_INFO_ERROR ) ;
            if ( NULL == pDetail || 0 == *pDetail )
               pDetail = "Failed to init for adding host" ;
            goto build_error_result ;
         }
         rc = runCmd.doit( retObj ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to do adding host[%s], rc = %d", pIP, rc ) ;
            tmpRc = omaGetStringElement ( retObj, OMA_FIELD_DETAIL, &pDetail ) ;
            if ( SDB_OK != tmpRc )
            {
               pDetail = pmdGetThreadEDUCB()->getInfo( EDU_INFO_ERROR ) ;
               if ( NULL == pDetail || 0 == *pDetail )
                  pDetail = "Not exeute js file yet" ;
            }
            goto build_error_result ;
         }
         rc = omaGetIntElement ( retObj, OMA_FIELD_ERRNO, errNum ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get errno from js after "
                    "adding host[%s], rc = %d", pIP, rc ) ;
            ss << "Failed to get errno from js after adding host[" <<
               pIP << "]" ;
            pDetail = ss.str().c_str() ;
            goto build_error_result ;
         }

         version = retObj.getStringField( OMA_FIELD_VERSION ) ;

         if ( SDB_OK != errNum )
         {
            rc = omaGetStringElement ( retObj, OMA_FIELD_DETAIL, &pDetail ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to get error detail from js after "
                       "adding host[%s], rc = %d", pIP, rc ) ;
               ss << "Failed to get error detail from js after adding host[" <<
                  pIP << "]" ;
               pDetail = ss.str().c_str() ;
               goto build_error_result ;
            }
            rc = errNum ;
            goto build_error_result ;
         }
         else
         {
            ossSnprintf( flow, OMA_BUFF_SIZE, "Finish adding host[%s]", pIP ) ;
            PD_LOG ( PDEVENT, "Success to add host[%s]", pIP ) ;
            resultInfo._status     = OMA_TASK_STATUS_FINISH ;
            resultInfo._statusDesc = getTaskStatusDesc( OMA_TASK_STATUS_FINISH ) ;
            resultInfo._version    = version ;
            resultInfo._flow.push_back( flow ) ;
            tmpRc = _pTask->updateProgressToTask( pInfo->_serialNum, resultInfo ) ;
            if ( tmpRc )
            {
               PD_LOG( PDWARNING, "Failed to update add host[%s]'s progress, "
                       "rc = %d", pIP, tmpRc ) ;
            }
         }
         continue ; // if we success, nerver go to "build_error_result"
         
      build_error_result:
         ossSnprintf( flow, OMA_BUFF_SIZE, "Failed to add host[%s]", pIP ) ;
         resultInfo._status     = OMA_TASK_STATUS_FINISH ;
         resultInfo._statusDesc = getTaskStatusDesc( OMA_TASK_STATUS_FINISH ) ;
         resultInfo._errno      = rc ;
         resultInfo._detail     = pDetail ;
         resultInfo._version    = version ;
         resultInfo._flow.push_back( flow ) ;
         tmpRc = _pTask->updateProgressToTask( pInfo->_serialNum, resultInfo ) ;
         if ( tmpRc )
         {
            PD_LOG( PDWARNING, "Failed to update add host[%s]'s progress, "
                    "rc = %d", pIP, tmpRc ) ;
         }
         continue ;
         
      }

   done:
      _pTask->setSubTaskStatus( _taskName, OMA_TASK_STATUS_FINISH ) ;
      _pTask->notifyUpdateProgress() ;
      return SDB_OK ;
   }

   /*
      install db business sub task
   */
   _omaInstDBBusSubTask::_omaInstDBBusSubTask( INT64 taskID )
   : _omaTask( taskID )
   {
      _taskType = OMA_TASK_INSTALL_DB_SUB ;
      _taskName = OMA_TASK_NAME_INSTALL_DB_BUSINESS_SUB ;
   }

   _omaInstDBBusSubTask::~_omaInstDBBusSubTask()
   {
   }


   INT32 _omaInstDBBusSubTask::init( const BSONObj &info, void *ptr )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      _pTask = (_omaInstDBBusTask *)ptr ;
      if ( NULL == _pTask )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "No task information for "
                     "installing db business sub task" ) ;
         goto error ;
      }
      ss << _taskName << "[" << _pTask->getSubTaskSerialNum() << "]" ;
      _taskName = ss.str() ;
      
      done:
         return rc ;
      error:
         goto done ;
   }

   INT32 _omaInstDBBusSubTask::doit()
   {
      INT32 rc                     = SDB_OK ;
      INT32 tmpRc                  = SDB_OK ;
      CHAR flow[OMA_BUFF_SIZE + 1] = { 0 } ;
      const CHAR *pDetail          = NULL ;
      const CHAR *pHostName        = NULL ;
      const CHAR *pSvcName         = NULL ;
      INT32 errNum                 = 0 ;
      OMA_TASK_STATUS taskStatus ;

      _pTask->setSubTaskStatus( _taskName, OMA_TASK_STATUS_RUNNING ) ;

      while( TRUE )
      {
         string instRGName ;

         if ( TRUE == pmdGetThreadEDUCB()->isInterrupted() )
         {
            PD_LOG( PDEVENT, "Program has been interrupted, stop task[%s]",
                    _taskName.c_str() ) ;
            goto done ;
         }
         if ( TRUE == _pTask->getIsTaskFail() )
         {
            PD_LOG( PDEVENT, "Install db business task had failed, "
                    "sub task[%s] exits", _taskName.c_str() ) ;
            goto done ;
         }
         instRGName = _pTask->getDataRGToInst() ;
         if ( instRGName.empty() )
         {
            PD_LOG( PDEVENT, "No data group need to install now, "
                    "sub task[%s] exits", _taskName.c_str() ) ;
            goto done ;
         }

         while( TRUE )
         {
            InstDBBusInfo *pInfo = NULL ;
            InstDBResult instResult ;
            stringstream ss ;
            BSONObj retObj ;

            pInfo = _pTask->getDataNodeInfo( instRGName ) ;
            if ( NULL == pInfo )
            {
               PD_LOG( PDEVENT, "Finish installing group[%s] in task[%s]",
                       instRGName.c_str(), _taskName.c_str() ) ;
               break ;
            }

            taskStatus = _pTask->getTaskStatus() ;
            if ( OMA_TASK_STATUS_RUNNING != taskStatus )
            {
               PD_LOG ( PDERROR, "Task's status is: [%d], stop running sub "
                        "task[%s]", taskStatus, _taskName.c_str() ) ;
               goto done ;
            }

            pHostName              = pInfo->_instInfo._hostName.c_str() ;
            pSvcName               = pInfo->_instInfo._svcName.c_str() ;
            instResult._errno      = SDB_OK ;
            instResult._detail     = "" ;
            instResult._hostName   = pHostName ;
            instResult._svcName    = pSvcName ;
            instResult._role       = ROLE_DATA ;
            instResult._groupName  = pInfo->_instInfo._dataGroupName ;
            instResult._status     = OMA_TASK_STATUS_RUNNING ;
            instResult._statusDesc = getTaskStatusDesc( OMA_TASK_STATUS_RUNNING ) ;

            ossSnprintf( flow, OMA_BUFF_SIZE, "Installing data node[%s:%s]",
                         pHostName, pSvcName ) ;
            instResult._status     = OMA_TASK_STATUS_RUNNING ;
            instResult._statusDesc = getTaskStatusDesc( OMA_TASK_STATUS_RUNNING ) ;
            instResult._flow.push_back( flow ) ;
            rc = _pTask->updateProgressToTask( pInfo->_nodeSerialNum,
                                               instResult, TRUE ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Failed to update install data node[%s:%s]'s "
                       "progress, rc = %d", pHostName, pSvcName, rc ) ;
            }
            _omaInstallDataNode runCmd( _taskID, _pTask->getTmpCoordSvcName(),
                                        pInfo->_instInfo ) ;
            rc = runCmd.init( NULL ) ;
            if ( rc )
            {
               
               PD_LOG( PDERROR, "Failed to init to install data node[%s:%s], "
                       "rc = %d", pHostName, pSvcName, rc ) ;
               pDetail = pmdGetThreadEDUCB()->getInfo( EDU_INFO_ERROR ) ;
               if ( NULL == pDetail || 0 == *pDetail )
                  pDetail = "Failed to init to install data node" ;
               goto build_error_result ;
            }
            rc = runCmd.doit( retObj ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to install data node[%s:%s], rc = %d",
                       pHostName, pSvcName, rc ) ;
               tmpRc = omaGetStringElement ( retObj, OMA_FIELD_DETAIL, &pDetail ) ;
               if ( SDB_OK != tmpRc )
               {
                  pDetail = pmdGetThreadEDUCB()->getInfo( EDU_INFO_ERROR ) ;
                  if ( NULL == pDetail || 0 == *pDetail )
                     pDetail = "Not exeute js file yet" ;
               }
               goto build_error_result ;
            }
            rc = omaGetIntElement ( retObj, OMA_FIELD_ERRNO, errNum ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to get errno from js after "
                       "installing data node[%s:%s], rc = %d",
                       pHostName, pSvcName, rc ) ;
               ss << "Failed to get errno from js after installing data node[" <<
                  pHostName << ":" << pSvcName << "]" ;
               pDetail = ss.str().c_str() ;
               goto build_error_result ;
            }
            if ( SDB_OK != errNum )
            {
               rc = omaGetStringElement ( retObj, OMA_FIELD_DETAIL, &pDetail ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to get error detail from js after "
                          "installing data node[%s:%s], rc = %d",
                          pHostName, pSvcName, rc ) ;
                  ss << "Failed to get error detail from js after installing "
                        "data node[" << pHostName << ":" << pSvcName << "]" ;
                  pDetail = ss.str().c_str() ;
                  goto build_error_result ;
               }
               rc = errNum ;
               goto build_error_result ;
            }
            else
            {
               ossSnprintf( flow, OMA_BUFF_SIZE, "Finish installing data "
                            "node[%s:%s]", pHostName, pSvcName ) ;
               PD_LOG ( PDEVENT, "Success to install data node[%s:%s]",
                        pHostName, pSvcName ) ;
               instResult._status     = OMA_TASK_STATUS_FINISH ;
               instResult._statusDesc = getTaskStatusDesc( OMA_TASK_STATUS_FINISH ) ;
               instResult._flow.push_back( flow ) ;
               rc = _pTask->updateProgressToTask( pInfo->_nodeSerialNum,
                                                  instResult, TRUE ) ;
               if ( rc )
               {
                  PD_LOG( PDWARNING, "Failed to update install data "
                          "node[%s:%s]'s progress, rc = %d",
                          pHostName, pSvcName, rc ) ;
               }
            }
            continue ; // if we success, nerver go to "build_error_result"
         
         build_error_result:
            ossSnprintf( flow, OMA_BUFF_SIZE, "Failed to install data "
                         "node[%s:%s], going to rollback",
                         pHostName, pSvcName ) ;
            instResult._status     = OMA_TASK_STATUS_ROLLBACK ;
            instResult._statusDesc = getTaskStatusDesc( OMA_TASK_STATUS_ROLLBACK ) ;
            instResult._errno      = rc ;
            instResult._detail     = pDetail ;
            instResult._flow.push_back( flow ) ;
            tmpRc = _pTask->updateProgressToTask( pInfo->_nodeSerialNum,
                                                  instResult, TRUE ) ;
            if ( SDB_OK != tmpRc )
            {
               PD_LOG( PDWARNING, "Failed to update install coord[%s:%s]'s "
                       "progress, rc = %d", pHostName, pSvcName, tmpRc ) ;
            }
            goto error ;
         } // while
         
      } // while

   done:
      _pTask->setSubTaskStatus( _taskName, OMA_TASK_STATUS_FINISH ) ;
      _pTask->notifyUpdateProgress() ;
      return rc ;
      
   error:
      _pTask->setIsTaskFail() ;
      _pTask->setErrInfo( rc, pDetail ) ;
      goto done ;
   }

   /*
      install zookeeper business sub task
   */
   _omaInstZNBusSubTask::_omaInstZNBusSubTask( INT64 taskID )
   : _omaTask( taskID )
   {
      _taskType = OMA_TASK_INSTALL_ZN_SUB ;
      _taskName = OMA_TASK_NAME_INSTALL_ZN_BUSINESS_SUB ;
   }

   _omaInstZNBusSubTask::~_omaInstZNBusSubTask()
   {
   }


   INT32 _omaInstZNBusSubTask::init( const BSONObj &info, void *ptr )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      _pTask = (_omaInstZNBusTask *)ptr ;
      if ( NULL == _pTask )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "No task information for "
                     "installing zookeeper business sub task" ) ;
         goto error ;
      }
      ss << _taskName << "[" << _pTask->getSubTaskSerialNum() << "]" ;
      _taskName = ss.str() ;
      
      done:
         return rc ;
      error:
         goto done ;
   }

   INT32 _omaInstZNBusSubTask::doit()
   {
      INT32 rc      = SDB_OK ;
      INT32 tmpRc   = SDB_OK ;
      const CHAR *pDetail          = NULL ;

      _pTask->setSubTaskStatus( _taskName, OMA_TASK_STATUS_RUNNING ) ;
      
      while( TRUE )
      {
         AddZNInfo *pInfo           = NULL ;
         AddZNResultInfo resultInfo = { "", "0", OMA_TASK_STATUS_RUNNING,
                                        OMA_TASK_STATUS_DESC_RUNNING,
                                        SDB_OK, "" } ;
         CHAR flow[OMA_BUFF_SIZE + 1] = { 0 } ;
         const CHAR *pHostName        = NULL ;
         INT32 errNum                 = 0 ;
         stringstream ss ;
         BSONObj retObj ;

         if ( TRUE == pmdGetThreadEDUCB()->isInterrupted() )
         {
            PD_LOG( PDEVENT, "Program has been interrupted, stop task[%s]",
                    _taskName.c_str() ) ;
            goto done ;
         }
         if ( TRUE == _pTask->getIsTaskFail() )
         {
            PD_LOG( PDEVENT, "Installing zookeeper business task had failed, "
                    "sub task[%s] exits", _taskName.c_str() ) ;
            goto done ;
         }
         pInfo = _pTask->getZNInfo() ;
         if ( NULL == pInfo )
         {
            PD_LOG( PDEVENT, "No znode needs to install now, sub task[%s] exits",
                    _taskName.c_str() ) ;
            goto done ;
         }

         pHostName            = pInfo->_item._hostName.c_str() ;
         resultInfo._hostName = pHostName ;
         resultInfo._zooid    = pInfo->_item._zooid ;

         ossSnprintf( flow, OMA_BUFF_SIZE, "Installing znode[%s]", pHostName ) ;
         resultInfo._flow.push_back( flow ) ;
         tmpRc = _pTask->updateProgressToTask( pInfo->_serialNum, resultInfo ) ;
         if ( tmpRc )
         {
            PD_LOG( PDWARNING, "Failed to update installing znode[%s]'s progress, "
                    "rc = %d", pHostName, tmpRc ) ;
         }

         _omaAddZNode runCmd( *pInfo ) ;
         rc = runCmd.init( NULL ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to init for installing "
                    "znode[%s], rc = %d", pHostName, rc ) ;
            pDetail = pmdGetThreadEDUCB()->getInfo( EDU_INFO_ERROR ) ;
            if ( NULL == pDetail || 0 == *pDetail )
               pDetail = "Failed to init for installing znode" ;
            goto build_error_result ;
         }
         rc = runCmd.doit( retObj ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to do installing znode[%s], "
                    "rc = %d", pHostName, rc ) ;
            tmpRc = omaGetStringElement ( retObj, OMA_FIELD_DETAIL, &pDetail ) ;
            if ( SDB_OK != tmpRc )
            {
               pDetail = pmdGetThreadEDUCB()->getInfo( EDU_INFO_ERROR ) ;
               if ( NULL == pDetail || 0 == *pDetail )
                  pDetail = "Not exeute js file yet" ;
            }
            goto build_error_result ;
         }
         rc = omaGetIntElement ( retObj, OMA_FIELD_ERRNO, errNum ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get errno from js after "
                    "installing znode[%s], rc = %d", pHostName, rc ) ;
            ss << "Failed to get errno from js after installing znode[" <<
               pHostName << "]" ;
            pDetail = ss.str().c_str() ;
            goto build_error_result ;
         }
         if ( SDB_OK != errNum )
         {
            rc = omaGetStringElement ( retObj, OMA_FIELD_DETAIL, &pDetail ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to get error detail from js after "
                       "installing znode[%s], rc = %d", pHostName, rc ) ;
               ss << "Failed to get error detail from js after installing "
                     "znode[" << pHostName << "]" ;
               pDetail = ss.str().c_str() ;
               goto build_error_result ;
            }
            rc = errNum ;
            goto build_error_result ;
         }
         else
         {
            ossSnprintf( flow, OMA_BUFF_SIZE, "Finish installing "
                         "znode[%s]", pHostName ) ;
            PD_LOG ( PDEVENT, "Success to add znode[%s]", pHostName ) ;
            resultInfo._status     = OMA_TASK_STATUS_FINISH ;
            resultInfo._statusDesc = getTaskStatusDesc( OMA_TASK_STATUS_FINISH ) ;
            resultInfo._flow.push_back( flow ) ;
            tmpRc = _pTask->updateProgressToTask( pInfo->_serialNum, resultInfo ) ;
            if ( tmpRc )
            {
               PD_LOG( PDWARNING, "Failed to update installing znode[%s]'s "
                       "progress, rc = %d", pHostName, tmpRc ) ;
            }
         }
         continue ; // if we success, nerver go to "build_error_result"
         
      build_error_result:
         ossSnprintf( flow, OMA_BUFF_SIZE, "Failed to install "
                      "znode[%s]", pHostName ) ;
         resultInfo._status     = OMA_TASK_STATUS_ROLLBACK ;
         resultInfo._statusDesc = getTaskStatusDesc( OMA_TASK_STATUS_ROLLBACK ) ;
         resultInfo._errno      = rc ;
         resultInfo._detail     = pDetail ;
         resultInfo._flow.push_back( flow ) ;
         tmpRc = _pTask->updateProgressToTask( pInfo->_serialNum, resultInfo ) ;
         if ( tmpRc )
         {
            PD_LOG( PDWARNING, "Failed to update installing znode[%s]'s "
                    "progress, rc = %d", pHostName, tmpRc ) ;
         }
         goto error ;
         
      } // while

   done:
      _pTask->setSubTaskStatus( _taskName, OMA_TASK_STATUS_FINISH ) ;
      _pTask->notifyUpdateProgress() ;
      return rc ;
      
   error:
      _pTask->setIsTaskFail() ;
      _pTask->setErrInfo( rc, pDetail ) ;
      goto done ;
   }

   _omaInstallSsqlOlapBusSubTask::_omaInstallSsqlOlapBusSubTask( INT64 taskID )
      : _omaTask( taskID )
   {
      _taskType = OMA_TASK_INSTALL_SSQL_OLAP_SUB ;
      _taskName = OMA_TASK_NAME_INSTALL_SSQL_OLAP_BUSINESS_SUB ;
   }

   _omaInstallSsqlOlapBusSubTask::~_omaInstallSsqlOlapBusSubTask()
   {
   }

   INT32 _omaInstallSsqlOlapBusSubTask::init( const BSONObj &info, void *ptr )
   {
      INT32 rc = SDB_OK ;

      _mainTask = (_omaInstallSsqlOlapBusTask*)ptr ;
      if ( NULL == _mainTask )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "No task information for "
                     "installing sequoiasql olap business sub task" ) ;
         goto error ;
      }

      {
         stringstream ss ;
         ss << _taskName << "[" << _mainTask->getSubTaskSerialNum() << "]" ;
         _taskName = ss.str() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omaInstallSsqlOlapBusSubTask::doit()
   {
      INT32 rc = SDB_OK ;
      INT32 tmpRc = SDB_OK ;
      string detail ;

      _mainTask->setSubTaskStatus( _taskName, OMA_TASK_STATUS_RUNNING ) ;

      while( TRUE )
      {
         omaSsqlOlapNodeInfo* nodeInfo = NULL ;
         string hostName ;
         string role ;
         INT32 errNum = 0 ;
         BSONObj retObj ;

         if ( pmdGetThreadEDUCB()->isInterrupted() )
         {
            PD_LOG( PDEVENT, "program has been interrupted, stop task[%s]",
                    _taskName.c_str() ) ;
            goto done ;
         }

         if ( _mainTask->isTaskFailed() )
         {
            PD_LOG( PDEVENT, "installing sequoiasql olap business task has been failed, "
                    "sub task[%s] exits", _taskName.c_str() ) ;
            goto done ;
         }

         nodeInfo = _mainTask->getNodeInfo() ;
         if ( NULL == nodeInfo )
         {
            PD_LOG( PDEVENT, "no sequoiasql olap node needs to install now, sub task[%s] exits",
                    _taskName.c_str() ) ;
            goto done ;
         }

         hostName = nodeInfo->hostName ;
         role = nodeInfo->role ;

         {
            stringstream ss ;
            ss << "installing " << hostName << ":" << role ;
            nodeInfo->status = OMA_TASK_STATUS_RUNNING ;
            nodeInfo->statusDesc = OMA_TASK_STATUS_DESC_RUNNING ;
            nodeInfo->flow.push_back( ss.str() ) ;
            INT32 tmpRc = _mainTask->updateProgressToTask( TRUE ) ;
            if ( SDB_OK != tmpRc )
            {
               PD_LOG( PDWARNING, "failed to update installing sequoiasql olap[%s:%s]'s progress, "
                       "rc = %d", hostName.c_str(), role.c_str(), tmpRc ) ;
            }
         }

         _omaInstallSsqlOlap runCmd( nodeInfo->config, _mainTask->getSysInfo() ) ;
         rc = runCmd.init( NULL ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to init for installing sequioasql olap[%s:%s], "
                    "rc = %d", hostName.c_str(), role.c_str(), rc ) ;
            detail = string( pmdGetThreadEDUCB()->getInfo( EDU_INFO_ERROR ) ) ;
            if ( "" == detail )
            {
               detail = "failed to init for installing sequioasql olap" ;
            }
            goto build_error_result ;
         }

         rc = runCmd.doit( retObj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to do installing sequoiasql olap[%s:%s], "
                    "rc = %d", hostName.c_str(), role.c_str(), rc ) ;
            tmpRc = omaGetStringElement ( retObj, OMA_FIELD_DETAIL, detail ) ;
            if ( SDB_OK != tmpRc )
            {
               detail = pmdGetThreadEDUCB()->getInfo( EDU_INFO_ERROR ) ;
               if ( "" == detail )
               {
                  detail = "not exeute js file yet" ;
               }
            }
            goto build_error_result ;
         }

         rc = omaGetIntElement( retObj, OMA_FIELD_ERRNO, errNum ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "failed to get errno from js after "
                    "installing sequoiasql olap[%s:%s], rc = %d",
                    hostName.c_str(), role.c_str(), rc ) ;
            stringstream ss ;
            ss << "failed to get errno from js after installing sequoiasql olap[" <<
               hostName << ":" << role << "]" ;
            detail = ss.str() ;
            goto build_error_result ;
         }

         if ( SDB_OK != errNum )
         {
            rc = omaGetStringElement ( retObj, OMA_FIELD_DETAIL, detail ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to get error detail from js after "
                       "installing sequoiasql olap[%s:%s], rc = %d",
                       hostName.c_str(), role.c_str(), rc ) ;
               stringstream ss ;
               ss << "failed to get error detail from js after installing "
                     "sequoiasql olap[" << hostName << ":" << role << "]" ;
               detail = ss.str() ;
               goto build_error_result ;
            }
            rc = errNum ;
            goto build_error_result ;
         }
         else
         {
            stringstream ss ;
            ss << "finish installing sequoiasql olap["
               << hostName << ":" << role << "]" ;
            PD_LOG ( PDEVENT, "successfully install sequoiasql olap[%s:%s]",
                     hostName.c_str(), role.c_str() ) ;

            nodeInfo->status = OMA_TASK_STATUS_FINISH ;
            nodeInfo->statusDesc = OMA_TASK_STATUS_DESC_FINISH ;
            nodeInfo->flow.push_back( ss.str() ) ;
            tmpRc = _mainTask->updateProgressToTask( TRUE ) ;
            if ( SDB_OK != tmpRc )
            {
               PD_LOG( PDWARNING, "failed to update installing sequoiasql olap[%s:%s]'s "
                       "progress, rc = %d", hostName.c_str(), role.c_str(), tmpRc ) ;
            }
         }
         continue ; // if we success, nerver go to "build_error_result"

      build_error_result:
         {
            stringstream ss ;
            ss << "failed to install sequoiasql olap["
               << hostName << ":" << role << "]" ;
            nodeInfo->status = OMA_TASK_STATUS_ROLLBACK ;
            nodeInfo->statusDesc = OMA_TASK_STATUS_DESC_ROLLBACK ;
            nodeInfo->errcode = rc ;
            nodeInfo->detail = detail ;
            nodeInfo->flow.push_back( ss.str() ) ;
            tmpRc = _mainTask->updateProgressToTask( TRUE ) ;
            if ( SDB_OK != tmpRc )
            {
               PD_LOG( PDWARNING, "failed to update installing sequoiasql olap[%s:%s]'s "
                       "progress, rc = %d", hostName.c_str(), role.c_str(), tmpRc ) ;
            }
         }
         goto error ;

      } // while

   done:
      _mainTask->setSubTaskStatus( _taskName, OMA_TASK_STATUS_FINISH ) ;
      _mainTask->notifyUpdateProgress() ;
      return rc ;

   error:
      _mainTask->setTaskFailed() ;
      _mainTask->setErrInfo( rc, detail ) ;
      goto done ;
   }

}  // namespace engine
