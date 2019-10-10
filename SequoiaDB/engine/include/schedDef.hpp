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

   Source File Name = schedDef.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/16/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SCHED_DEF_HPP__
#define SCHED_DEF_HPP__

#include <string>
#include "oss.hpp"
#include "ossAtomic.hpp"
#include "ossEvent.hpp"
#include "../bson/bson.h"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      COMMON DEFINE
   */
   #define SCHED_SYS_CONTAINER_NAME                   "SYSTEM"
   #define SCHED_TASK_NAME_DFT                        "Default"

   #define SCHED_NICE_DFT                             ( 0 )
   #define SCHED_NICE_MAX                             ( 19 )
   #define SCHED_NICE_MIN                             ( -20 )

   #define SCHED_TASK_ID_DFT                          ( 0 )

   #define SCHED_TASK_NAME_LEN                        ( 127 )
   #define SCHED_CONTIANER_NAME_LEN                   ( 127 )
   #define SCHED_USER_NAME_LEN                        ( 63 )
   #define SCHED_IP_STR_LEN                           ( 31 )

   /*
      SCHED_TYPE define
   */
   enum SCHED_TYPE
   {
      SCHED_TYPE_NONE      = 0,
      SCHED_TYPE_FIFO,              // FIFO scheduler
      SCHED_TYPE_PRIORITY,          // priority scheduler
      SCHED_TYPE_CONTAINER,         // container scheduler

      SCHED_TYPE_MAX
   } ;
   #define SCHED_TYPE_DEFAULT          SCHED_TYPE_NONE

   const CHAR* schedType2String( SCHED_TYPE type ) ;
   SCHED_TYPE  schedString2Type( const CHAR *pStr ) ;

   /*
      _schedInfo define
   */
   class _schedInfo : public SDBObject
   {
      public:
         _schedInfo() ;
         ~_schedInfo() ;

         BSONObj  toBSON() const ;
         INT32    fromBSON( const BSONObj &obj ) ;

         void     reset() ;

         INT32    getNice() const { return _nice ; }
         INT64    getTaskID() const { return _taskID ; }

         const CHAR*    getTaskName() const { return _taskName ; }
         const CHAR*    getContianerName() const { return _containerName ; }
         const CHAR*    getIP() const { return _ip ; }
         const CHAR*    getUserName() const { return _userName ; }

         void     setNice( INT32 nice ) ;
         void     setTaskID( INT64 taskID ) ;
         void     setTaskName( const CHAR* taskName ) ;
         void     setContainerName( const CHAR* containerName ) ;
         void     setIP( const CHAR* ip ) ;
         void     setUserName( const CHAR* userName ) ;

      private:
         INT32                _nice ;
         INT64                _taskID ;

         CHAR                 _taskName[ SCHED_TASK_NAME_LEN + 1 ] ;
         CHAR                 _containerName[ SCHED_CONTIANER_NAME_LEN + 1 ] ;
         CHAR                 _userName[ SCHED_USER_NAME_LEN + 1 ] ;
         CHAR                 _ip[ SCHED_IP_STR_LEN + 1 ] ;

   } ;
   typedef _schedInfo schedInfo ;

   /*
      _schedTaskInfo define
   */
   class _schedTaskInfo : public SDBObject
   {
      public:
         _schedTaskInfo() ;
         ~_schedTaskInfo() ;

         void     setTaskLimit( UINT32 limit ) ;

         void     beginATask() ;
         void     doneATask() ;

         UINT32   getRemaingTask() ;
         INT32    waitRemaingTask( INT64 millisec ) ;

         INT32    getAndWaitRemaingTask( INT64 millisec, UINT32 &num ) ;

         UINT32   getRunTaskNum() ;

      private:
         ossAtomic32                _runTaskNum ;
         UINT32                     _limitTaskNum ;
         ossEvent                   _event ;
   } ;
   typedef _schedTaskInfo schedTaskInfo ;

}

#endif // SCHED_DEF_HPP__
