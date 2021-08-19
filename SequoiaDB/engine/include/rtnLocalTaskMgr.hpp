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

   Source File Name = rtnLocalTaskMgr.hpp

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
#ifndef RTN_LOCAL_TASK_MGR_HPP__
#define RTN_LOCAL_TASK_MGR_HPP__

#include "rtnLocalTaskFactory.hpp"
#include "pmdEDU.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   /*
      _rtnLocalTaskMgr define
   */
   class _rtnLocalTaskMgr : public SDBObject
   {
      public:
         typedef ossPoolMap<UINT64, rtnLocalTaskPtr>     MAP_TASK ;
         typedef MAP_TASK::iterator                      MAP_TASK_IT ;

      public:
         _rtnLocalTaskMgr () ;
         ~_rtnLocalTaskMgr () ;

         INT32       reload( pmdEDUCB *cb ) ;
         void        fini() ;
         void        clear() ;

      public:
         UINT32      taskCount () ;
         UINT32      taskCount( RTN_LOCAL_TASK_TYPE type ) ;

         INT32       waitTaskEvent( INT64 millisec = OSS_ONE_SEC ) ;

         INT32       addTask ( rtnLocalTaskPtr &ptr,
                               pmdEDUCB *cb,
                               _dpsLogWrapper *dpsCB ) ;
         void        removeTask ( UINT64 taskID,
                                  pmdEDUCB *cb,
                                  _dpsLogWrapper *dpsCB ) ;
         void        removeTask( const rtnLocalTaskPtr &ptr,
                                 pmdEDUCB *cb,
                                 _dpsLogWrapper *dpsCB ) ;

         INT32       dumpTask( MAP_TASK &mapTask ) ;

         INT32       dumpTask( RTN_LOCAL_TASK_TYPE type,
                               MAP_TASK &mapTask ) ;

      protected:
         void        _clear() ;

      private:
         MAP_TASK                            _taskMap ;
         ossSpinSLatch                       _taskLatch ;
         ossAutoEvent                        _taskEvent ;

         UINT64                              _taskID ;

   };
   typedef _rtnLocalTaskMgr rtnLocalTaskMgr ;

}

#endif //RTN_LOCAL_TASK_MGR_HPP__

