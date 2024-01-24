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

   Source File Name = rtnLocalTaskMgr.cpp

   Descriptive Name = Local Task Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS Temporary Storage Unit Management.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/27/2020  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnLocalTaskMgr.hpp"
#include "dmsLocalSUMgr.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"
#include "dmsCB.hpp"
#include "clsMgr.hpp"

using namespace bson ;

namespace engine
{

   /*
      _rtnLocalTaskMgr implement
   */
   _rtnLocalTaskMgr::_rtnLocalTaskMgr()
   {
      _taskID = RTN_LT_INVALID_TASKID + 1 ;
   }

   _rtnLocalTaskMgr::~_rtnLocalTaskMgr()
   {
      fini() ;
   }

   INT32 _rtnLocalTaskMgr::reload( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      _SDB_RTNCB *rtnCB = krcb->getRTNCB() ;
      dmsLocalSUMgr *pMgr = krcb->getDMSCB()->getLocalSUMgr() ;
      ossPoolVector< BSONObj > vecObj ;
      BSONElement e ;
      UINT64 taskID = RTN_LT_INVALID_TASKID ;
      INT32 taskType = 0 ;
      rtnLocalTaskPtr ptr ;

      ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;

      /// first clear
      _clear() ;

      rc = pMgr->queryTask( BSONObj(), vecObj, rtnCB, cb, -1 ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Query local task failed, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         for ( UINT32 i = 0 ; i < vecObj.size() ; ++i )
         {
            BSONObj &obj = vecObj[ i ] ;
            /// first get _id
            e = obj.getField( DMS_ID_KEY_NAME ) ;
            if ( !e.isNumber() )
            {
               PD_LOG( PDWARNING, "Task(%s) 's _id is invalid",
                       obj.toString().c_str() ) ;
               continue ;
            }

            taskID = e.numberLong() ;
            if ( taskID >= _taskID )
            {
               _taskID = taskID + 1 ;
            }

            /// get task type
            e = obj.getField( RTN_LT_FIELD_TASKTYPE ) ;
            if ( NumberInt != e.type() )
            {
               PD_LOG( PDWARNING, "Task(%s) is invalid, not found field[%s]",
                       obj.toString().c_str(), RTN_LT_FIELD_TASKTYPE ) ;
               continue ;
            }
            taskType = e.numberInt() ;

            /// create task
            rc = rtnGetLTFactory()->create( taskType, ptr ) ;
            if ( SDB_INVALIDARG == rc )
            {
               PD_LOG( PDWARNING, "Task(%s)'s type is invalid",
                       obj.toString().c_str() ) ;
               continue ;
            }
            else if ( rc )
            {
               PD_LOG( PDERROR, "Create task(%s) failed, rc: %d",
                       obj.toString().c_str(), rc ) ;
               goto error ;
            }

            rc = ptr->initFromBson( obj ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to initialize task from BSON, "
                         "rc: %d", rc ) ;

            /// add task
            ptr->_setTaskID( taskID ) ;
            _taskMap[ taskID ] = ptr ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _taskEvent.reset() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnLocalTaskMgr::fini()
   {
      _clear() ;
   }

   void _rtnLocalTaskMgr::clear()
   {
      ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;
      _clear() ;
   }

   void _rtnLocalTaskMgr::_clear()
   {
      _taskMap.clear() ;
      _taskEvent.signal() ;
   }

   UINT32 _rtnLocalTaskMgr::taskCount ()
   {
      ossScopedLock lock ( &_taskLatch, SHARED ) ;
      return (UINT32)_taskMap.size() ;
   }

   UINT32 _rtnLocalTaskMgr::taskCount( RTN_LOCAL_TASK_TYPE type )
   {
      UINT32 taskCount = 0 ;

      ossScopedLock lock ( &_taskLatch, SHARED ) ;

      MAP_TASK_IT it = _taskMap.begin() ;
      while ( it != _taskMap.end() )
      {
         if ( type == it->second->getTaskType() )
         {
            ++taskCount ;
         }
         ++it ;
      }

      return taskCount ;
   }

   INT32 _rtnLocalTaskMgr::waitTaskEvent( INT64 millisec )
   {
      return _taskEvent.wait( millisec ) ;
   }

   INT32 _rtnLocalTaskMgr::addTask ( rtnLocalTaskPtr &ptr,
                                     pmdEDUCB *cb,
                                     _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;
      UINT64 taskID = RTN_LT_INVALID_TASKID ;
      rtnLocalTaskPtr indexPtr ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      dmsLocalSUMgr *pMgr = krcb->getDMSCB()->getLocalSUMgr() ;
      INT16 w = pmdGetOptionCB()->transReplSize() ;
      INT16 finalW = 0 ;
      BSONObj obj ;
      BSONObj retIDObj ;
      UINT64 lastWriteCount = 0 ;

      w = w <= 0 ? 2 : w ;
      rc = pmdGetKRCB()->getClsCB()->getReplCB()->replSizeCheck( w, finalW, cb ) ;
      if ( SDB_OK != rc )
      {
         finalW = w ;
         PD_LOG( PDWARNING, "Failed to check repl size for task[%s], given "
                 "repl size [%d], rc: %d", ptr->toPrintString().c_str(), w, rc ) ;
         rc = SDB_OK ;
      }

      {
         ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;

         MAP_TASK_IT it = _taskMap.begin () ;
         while ( it != _taskMap.end() )
         {
            indexPtr = it->second ;

            if ( ptr->muteXOn( indexPtr.get() ) ||
                 indexPtr->muteXOn( ptr.get() ) )
            {
               PD_LOG ( PDWARNING, "Exist task[%s] mutex with new task[%s]",
                        indexPtr->toPrintString().c_str(),
                        ptr->toPrintString().c_str() ) ;
               rc = SDB_CLS_MUTEX_TASK_EXIST ;
               goto error ;
            }
            ++it ;
         }

         /// Alloc taskID
         while ( TRUE )
         {
            taskID = _taskID++ ;
            if ( _taskMap.find( taskID ) == _taskMap.end() )
            {
               break ;
            }
         }
         ptr->_setTaskID( taskID ) ;

         rc = ptr->toBson( obj ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Task to bson failed, rc: %d", rc ) ;
            goto error ;
         }

         // add to map
         try
         {
            _taskMap[ taskID ] = ptr ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            ptr->_setTaskID( RTN_LT_INVALID_TASKID ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }

      lastWriteCount = cb->getMonAppCB()->totalDataWrite ;
      // save to file
      rc = pMgr->addTask( obj, cb, dpsCB, finalW, retIDObj ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Add task failed, rc: %d", rc ) ;

         /// need rollback
         if ( lastWriteCount != cb->getMonAppCB()->totalDataWrite )
         {
            if ( SDB_OK == pMgr->removeTask( retIDObj, cb, dpsCB, 1 ) )
            {
               ptr->_setTaskID( RTN_LT_INVALID_TASKID ) ;
            }
         }

         ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;
         _taskMap.erase( taskID ) ;

         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnLocalTaskMgr::removeTask ( UINT64 taskID,
                                       pmdEDUCB *cb,
                                       _dpsLogWrapper *dpsCB )
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      dmsLocalSUMgr *pMgr = krcb->getDMSCB()->getLocalSUMgr() ;
      BOOLEAN hasDel = FALSE ;

      try
      {
         if ( SDB_OK == pMgr->removeTask( BSON( DMS_ID_KEY_NAME << (INT64)taskID ),
                                          cb, dpsCB, 1 ) )
         {
            hasDel = TRUE ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Remove task(%llu) occur exception: %s",
                 taskID, e.what() ) ;
      }

      {
         ossScopedLock lock ( &_taskLatch, EXCLUSIVE ) ;

         MAP_TASK_IT it = _taskMap.find ( taskID ) ;
         if ( it != _taskMap.end() )
         {
            if ( hasDel )
            {
               it->second->_setTaskID( RTN_LT_INVALID_TASKID ) ;
            }
            _taskMap.erase ( it ) ;
            _taskEvent.signal() ;
         }
      }
   }

   void _rtnLocalTaskMgr::removeTask( const rtnLocalTaskPtr &ptr,
                                      pmdEDUCB *cb,
                                      _dpsLogWrapper *dpsCB )
   {
      if ( ptr.get() )
      {
         removeTask( ptr->getTaskID(), cb, dpsCB ) ;
      }
   }

   INT32 _rtnLocalTaskMgr::dumpTask( _rtnLocalTaskMgr::MAP_TASK &mapTask )
   {
      INT32 rc = SDB_OK ;

      ossScopedLock lock ( &_taskLatch, SHARED ) ;

      try
      {
         mapTask = _taskMap ;
      }
      catch( std::exception &e )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return rc ;
   }

   INT32 _rtnLocalTaskMgr::dumpTask( RTN_LOCAL_TASK_TYPE type,
                                     _rtnLocalTaskMgr::MAP_TASK &mapTask )
   {
      INT32 rc = SDB_OK ;
      MAP_TASK_IT it ;

      ossScopedLock lock ( &_taskLatch, SHARED ) ;

      try
      {
         it = _taskMap.begin() ;
         while( it != _taskMap.end() )
         {
            if ( type == it->second->getTaskType() )
            {
               mapTask[ it->first ] = it->second ;
            }
            ++it ;
         }
      }
      catch( std::exception &e )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return rc ;
   }

   rtnLocalTaskPtr _rtnLocalTaskMgr::getTask( UINT64 taskID )
   {
      rtnLocalTaskPtr taskPtr ;

      ossScopedLock lock( &_taskLatch, SHARED ) ;

      MAP_TASK_IT iter = _taskMap.find( taskID ) ;
      if ( _taskMap.end() != iter )
      {
         taskPtr = iter->second ;
      }

      return taskPtr ;
   }

}


