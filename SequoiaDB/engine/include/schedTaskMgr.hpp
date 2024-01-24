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

   Source File Name = schedTaskMgr.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/29/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SCHED_TASK_MGR_HPP__
#define SCHED_TASK_MGR_HPP__

#include "monCB.hpp"
#include "monLatch.hpp"
#include "schedDef.hpp"
#include <boost/shared_ptr.hpp>
#include "ossMemPool.hpp"

using namespace std ;

namespace engine
{

   typedef boost::shared_ptr<monSvcTaskInfo>       monSvcTaskInfoPtr ;
   typedef ossPoolMap< UINT64, monSvcTaskInfoPtr > MAP_SVCTASKINFO_PTR ;
   typedef MAP_SVCTASKINFO_PTR::iterator           MAP_SVCTASKINFO_PTR_IT ;

   /*
      _schedItem define
   */
   struct _schedItem
   {
      schedInfo            _info ;
      monSvcTaskInfoPtr    _ptr ;

      void reset()
      {
         _info.reset() ;
         _ptr = monSvcTaskInfoPtr() ;
      }
   } ;
   typedef _schedItem schedItem ;

   /*
      _schedTaskMgr define
   */
   class _schedTaskMgr : public SDBObject
   {
      public:
         _schedTaskMgr() ;
         ~_schedTaskMgr() ;

         INT32       init() ;
         void        fini() ;

         monSvcTaskInfoPtr    getTaskInfoPtr( UINT64 taskID,
                                              const CHAR *taskName ) ;

         void        reset() ;

         MAP_SVCTASKINFO_PTR  getTaskInfos() ;

      private:
         monSvcTaskInfoPtr                   _defaultPtr ;
         MAP_SVCTASKINFO_PTR                 _mapTaskInfo ;
         monSpinXLatch                       _latch ;

   } ;
   typedef _schedTaskMgr schedTaskMgr ;

}

#endif // SCHED_TASK_MGR_HPP__
