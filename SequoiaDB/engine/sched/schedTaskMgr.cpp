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

   Source File Name = schedTaskMgr.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/29/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "schedTaskMgr.hpp"
#include "schedDef.hpp"
#include "pd.hpp"

namespace engine
{

   /*
      _schedTaskMgr implement
   */
   _schedTaskMgr::_schedTaskMgr()
   :_latch( MON_LATCH_SCHEDTASKMGR_LATCH )
   {
   }

   _schedTaskMgr::~_schedTaskMgr()
   {
      fini() ;
   }

   INT32 _schedTaskMgr::init()
   {
      INT32 rc = SDB_OK ;
      monSvcTaskInfo *pDefault = NULL ;

      pDefault = SDB_OSS_NEW monSvcTaskInfo() ;
      if ( !pDefault )
      {
         PD_LOG( PDERROR, "Allocate task info failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      pDefault->setTaskInfo( SCHED_TASK_ID_DFT, SCHED_TASK_NAME_DFT ) ;
      _defaultPtr = monSvcTaskInfoPtr( pDefault ) ;
      pDefault = NULL ;

      /// add to map
      _mapTaskInfo[ _defaultPtr->getTaskID() ] = _defaultPtr ;

      /// add to default
      monSetDefaultTaskInfo( _defaultPtr.get() ) ;

   done:
      if ( pDefault )
      {
         SDB_OSS_DEL pDefault ;
      }
      return rc ;
   error:
      goto done ;
   }

   void _schedTaskMgr::fini()
   {
      /// del from default
      monSetDefaultTaskInfo( NULL ) ;

      /// clear map
      _mapTaskInfo.clear() ;
      _defaultPtr = monSvcTaskInfoPtr() ;
   }

   monSvcTaskInfoPtr _schedTaskMgr::getTaskInfoPtr( UINT64 taskID,
                                                    const CHAR *taskName )
   {
      monSvcTaskInfoPtr ptr = _defaultPtr ;

      if ( taskID == _defaultPtr->getTaskID() )
      {
         ptr = _defaultPtr ;
      }
      else
      {
         MAP_SVCTASKINFO_PTR_IT it ;
         ossScopedLock lock( &_latch ) ;

         it = _mapTaskInfo.find( taskID ) ;
         if ( it != _mapTaskInfo.end() )
         {
            ptr = it->second ;

            /// Update name
            if ( 0 != ossStrcmp( ptr->getTaskName(), taskName ) )
            {
               ptr->setTaskInfo( taskID, taskName ) ;
            }
         }
         else
         {
            /// create new
            monSvcTaskInfo *pTaskInfo = SDB_OSS_NEW monSvcTaskInfo() ;
            if ( pTaskInfo )
            {
               pTaskInfo->setTaskInfo( taskID, taskName ) ;
               ptr = monSvcTaskInfoPtr( pTaskInfo ) ;
               /// add to map
               _mapTaskInfo[ ptr->getTaskID() ] = ptr ;
            }
            else
            {
               ptr = _defaultPtr ;
            }
         }
      }

      return ptr ;
   }

   void _schedTaskMgr::reset()
   {
      /// erase ptr when use_count() == 1
      MAP_SVCTASKINFO_PTR_IT it ;
      ossScopedLock lock( &_latch ) ;

      it = _mapTaskInfo.begin() ;
      while( it != _mapTaskInfo.end() )
      {
         if ( it->second.use_count() == 1 )
         {
            _mapTaskInfo.erase( it++ ) ;
            continue ;
         }
         it->second->reset() ;
         ++it ;
      }
   }

   MAP_SVCTASKINFO_PTR _schedTaskMgr::getTaskInfos()
   {
      ossScopedLock lock( &_latch ) ;
      return _mapTaskInfo ;
   }

}

