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

   Source File Name = schedDef.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/16/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "schedDef.hpp"
#include "msgDef.h"
#include "ossUtil.hpp"
#include "pd.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   #define SCHED_REMAINING_NUM_DFT              ( 500 )

   #define SCHED_TYPE_NONE_STR                  "NONE"
   #define SCHED_TYPE_FIFO_STR                  "FIFO"
   #define SCHED_TYPE_PRIORITY_STR              "PRIORITY"
   #define SCHED_TYPE_CONTAINER_STR             "CONTAINER"

   /*
      Common functions
   */
   const CHAR* schedType2String( SCHED_TYPE type )
   {
      const CHAR *pStr = "Unknow" ;

      switch ( type )
      {
         case SCHED_TYPE_NONE:
            pStr = SCHED_TYPE_NONE_STR ;
            break ;
         case SCHED_TYPE_FIFO :
            pStr = SCHED_TYPE_FIFO_STR ;
            break ;
         case SCHED_TYPE_PRIORITY :
            pStr = SCHED_TYPE_PRIORITY_STR ;
            break ;
         case SCHED_TYPE_CONTAINER :
            pStr = SCHED_TYPE_CONTAINER_STR ;
            break ;
         default:
            break ;
      }

      return pStr ;
   }

   SCHED_TYPE schedString2Type( const CHAR *pStr )
   {
      SCHED_TYPE type = SCHED_TYPE_DEFAULT ;

      if ( pStr )
      {
         if ( 0 == ossStrcasecmp( pStr, SCHED_TYPE_NONE_STR ) )
         {
            type = SCHED_TYPE_NONE ;
         }
         else if ( 0 == ossStrcasecmp( pStr, SCHED_TYPE_FIFO_STR ) )
         {
            type = SCHED_TYPE_FIFO ;
         }
         else if ( 0 == ossStrcasecmp( pStr, SCHED_TYPE_PRIORITY_STR ) )
         {
            type = SCHED_TYPE_PRIORITY ;
         }
         else if ( 0 == ossStrcasecmp( pStr, SCHED_TYPE_CONTAINER_STR ) )
         {
            type = SCHED_TYPE_CONTAINER ;
         }
      }

      return type ;
   }

   /*
      _schedInfo implement
   */
   _schedInfo::_schedInfo()
   :_nice( SCHED_NICE_DFT ),
    _taskID( SCHED_TASK_ID_DFT )
   {
      ossMemset( _taskName, 0, sizeof( _taskName ) ) ;
      ossMemset( _containerName, 0, sizeof( _containerName ) ) ;
      ossMemset( _userName, 0, sizeof( _userName ) ) ;
      ossMemset( _ip, 0, sizeof( _ip ) ) ;

      setTaskName( SCHED_TASK_NAME_DFT ) ;
      setContainerName( SCHED_SYS_CONTAINER_NAME ) ;
   }

   _schedInfo::~_schedInfo()
   {
   }

   void _schedInfo::reset()
   {
      _nice = SCHED_NICE_DFT ;
      _taskID = SCHED_TASK_ID_DFT ;

      setTaskName( SCHED_TASK_NAME_DFT ) ;
      setContainerName( SCHED_SYS_CONTAINER_NAME ) ;
   }

   BSONObj _schedInfo::toBSON() const
   {
      BSONObjBuilder bobObj ;
      bobObj.append( FIELD_NAME_NICE, _nice ) ;
      bobObj.append( FIELD_NAME_TASKID, _taskID ) ;
      bobObj.append( FIELD_NAME_TASK_NAME, _taskName ) ;
      bobObj.append( FIELD_NAME_CONTAINER_NAME, _containerName ) ;
      bobObj.append( FIELD_NAME_USER, _userName ) ;
      bobObj.append( FIELD_NAME_IP, _ip ) ;

      return bobObj.obj() ;
   }

   INT32 _schedInfo::fromBSON( const BSONObj &obj )
   {
      BSONElement beField ;

      beField = obj.getField( FIELD_NAME_NICE ) ;
      if( beField.isNumber() )
      {
         setNice( beField.numberInt() ) ;
      }

      beField= obj.getField( FIELD_NAME_TASKID ) ;
      if( beField.isNumber() )
      {
         setTaskID( beField.numberLong() ) ;
      }

      beField= obj.getField( FIELD_NAME_TASK_NAME ) ;
      if( beField.type() == String )
      {
         setTaskName( beField.valuestr() ) ;
      }

      beField= obj.getField( FIELD_NAME_CONTAINER_NAME ) ;
      if( beField.type() == String )
      {
         setContainerName( beField.valuestr() ) ;
      }

      beField= obj.getField( FIELD_NAME_USER ) ;
      if( beField.type() == String )
      {
         setUserName( beField.valuestr() ) ;
      }

      beField= obj.getField( FIELD_NAME_IP ) ;
      if( beField.type() == String )
      {
         setIP( beField.valuestr() ) ;
      }

      return SDB_OK ;
   }

   void _schedInfo::setNice( INT32 nice )
   {
      if ( nice > SCHED_NICE_MAX )
      {
         _nice = SCHED_NICE_MAX ;
      }
      else if ( nice < SCHED_NICE_MIN )
      {
         _nice = SCHED_NICE_MIN ;
      }
      else
      {
         _nice = nice ;
      }
   }

   void _schedInfo::setTaskID( INT64 taskID )
   {
      _taskID = taskID ;
   }

   void _schedInfo::setTaskName( const CHAR* taskName )
   {
      ossStrncpy( _taskName, taskName, SCHED_TASK_NAME_LEN ) ;
   }

   void _schedInfo::setContainerName( const CHAR* containerName )
   {
      ossStrncpy( _containerName, containerName, SCHED_CONTIANER_NAME_LEN ) ;
   }

   void _schedInfo::setIP( const CHAR *ip )
   {
      ossStrncpy( _ip, ip, SCHED_IP_STR_LEN ) ;
   }

   void _schedInfo::setUserName( const CHAR* userName )
   {
      ossStrncpy( _userName, userName, SCHED_USER_NAME_LEN ) ;
   }

   /*
      _schedTaskInfo implement
   */
   _schedTaskInfo::_schedTaskInfo()
   :_runTaskNum( 0 ), _limitTaskNum( 0 )
   {
   }

   _schedTaskInfo::~_schedTaskInfo()
   {
   }

   void _schedTaskInfo::setTaskLimit( UINT32 limit )
   {
      _limitTaskNum = limit ;
   }

   UINT32 _schedTaskInfo::getRunTaskNum()
   {
      return _runTaskNum.fetch() ;
   }

   void _schedTaskInfo::beginATask()
   {
      _runTaskNum.inc() ;
   }

   void _schedTaskInfo::doneATask()
   {
      UINT32 preRunTask = _runTaskNum.dec() ;

      if ( _limitTaskNum > 0 )
      {
         _event.signalAll() ;
      }
   }

   UINT32 _schedTaskInfo::getRemaingTask()
   {
      INT32 remaining = SCHED_REMAINING_NUM_DFT ;

      if ( _limitTaskNum > 0 )
      {
         remaining = ( INT32 )_limitTaskNum - _runTaskNum.fetch() ;
      }

      return remaining > 0 ? remaining : 0 ;
   }

   INT32 _schedTaskInfo::waitRemaingTask( INT64 millisec )
   {
      INT32 rc = SDB_OK ;

      if ( _limitTaskNum > 0 )
      {
         _event.reset() ;

         if ( ( INT32 )_limitTaskNum - _runTaskNum.fetch() <= 0 )
         {
            rc = _event.wait( millisec ) ;
         }
         
      }

      return rc ;
   }

   INT32 _schedTaskInfo::getAndWaitRemaingTask( INT64 millisec, UINT32 &num )
   {
      INT32 rc = SDB_OK ;

      num = getRemaingTask() ;
      while( num <= 0 )
      {
         rc = waitRemaingTask( millisec ) ;
         if ( rc )
         {
            break ;
         }
         num = getRemaingTask() ;
      }

      return rc ;
   }

}

