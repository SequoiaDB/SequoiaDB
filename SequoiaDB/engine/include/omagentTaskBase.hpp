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

   Source File Name = omagentTaskBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/30/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENTTASKBASE_HPP_
#define OMAGENTTASKBASE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossLatch.hpp"
#include "../bson/bson.h"
#include "omagentMsgDef.hpp"
#include "omagent.hpp"
#include <map>
#include <vector>
#include <string>

using namespace std ;
using namespace bson ;

#define OMA_INVALID_TASKID     (0)

namespace engine
{
   enum OMA_TASK_TYPE
   {
      OMA_TASK_TYPE_BEGIN         = -1,

      OMA_TASK_ADD_HOST           = OM_TASK_TYPE_ADD_HOST,

      OMA_TASK_REMOVE_HOST        = OM_TASK_TYPE_REMOVE_HOST,

      OMA_TASK_ADD_BUS            = OM_TASK_TYPE_ADD_BUSINESS,

      OMA_TASK_REMOVE_BUS         = OM_TASK_TYPE_REMOVE_BUSINESS,

      OMA_TASK_SSQL_EXEC          = OM_TASK_TYPE_SSQL_EXEC,

      OMA_TASK_EXTEND_BUZ         = OM_TASK_TYPE_EXTEND_BUSINESS,

      OMA_TASK_SHRINK_BUSINESS    = OM_TASK_TYPE_SHRINK_BUSINESS,

      OMA_TASK_DEPLOY_PACKAGE     = OM_TASK_TYPE_DEPLOY_PACKAGE,

      OMA_TASK_TYPE_END,
      
      OMA_TASK_SUB_TYPE_BEGIN,
      OMA_TASK_INSTALL_DB         = 10,
      OMA_TASK_REMOVE_DB          = 11,
      OMA_TASK_INSTALL_ZN         = 12, // install zookeeper business
      OMA_TASK_REMOVE_ZN          = 13, // remove zookeeper business
      OMA_TASK_INSTALL_SSQL_OLAP  = 14,
      OMA_TASK_REMOVE_SSQL_OLAP   = 15,
      OMA_TASK_EXTEND_DB          = 16,
      OMA_TASK_SUB_TYPE_END,


      OMA_TASK_TYPE_AGENT_BEGIN,
      OMA_TASK_ADD_HOST_SUB          = 20, // add host sub task
      OMA_TASK_INSTALL_DB_SUB        = 21, // install db business sub task
      OMA_TASK_INSTALL_ZN_SUB        = 22, // install zookeeper sub task
      OMA_TASK_INSTALL_SSQL_OLAP_SUB = 23, // install sequoiasql olap sub task
      OMA_TASK_ASYNC_SUB             = 24, // exec plan async sub task
      OMA_TASK_TYPE_AGENT_END,

      OMA_TASK_END
   } ;

   enum OMA_TASK_STATUS
   {
      OMA_TASK_STATUS_INIT        = OM_TASK_STATUS_INIT ,
      OMA_TASK_STATUS_RUNNING     = OM_TASK_STATUS_RUNNING ,
      OMA_TASK_STATUS_ROLLBACK    = OM_TASK_STATUS_ROLLBACK ,
      OMA_TASK_STATUS_CANCEL      = OM_TASK_STATUS_CANCEL ,
      OMA_TASK_STATUS_FINISH      = OM_TASK_STATUS_FINISH ,
      
      OMA_TASK_STATUS_FAIL        = 10 , // this status only use in omagent task
      OMA_TASK_STATUS_END
   } ;

   #define OMA_TASK_STATUS_DESC_INIT     OM_TASK_STATUS_INIT_STR
   #define OMA_TASK_STATUS_DESC_RUNNING  OM_TASK_STATUS_RUNNING_STR
   #define OMA_TASK_STATUS_DESC_ROLLBACK OM_TASK_STATUS_ROLLBACK_STR
   #define OMA_TASK_STATUS_DESC_CANCEL   OM_TASK_STATUS_CANCEL_STR
   #define OMA_TASK_STATUS_DESC_FINISH   OM_TASK_STATUS_FINISH_STR
   #define OMA_TASK_STATUS_DESC_UNKNOWN  "UNKNOWN"

   const CHAR* getTaskStatusDesc( OMA_TASK_STATUS status ) ;

   /*
      omagent task
   */
   class _omaTask : public SDBObject
   {
      public:
         _omaTask ( INT64 taskID ) : _taskID ( taskID )
         {
            _taskID           = taskID ;
            _taskStatus       = OMA_TASK_STATUS_INIT ;
            _subTaskSerialNum = 0 ;
            _eduID            = 0 ;
         }
         virtual ~_omaTask () {}

      public:
         OMA_TASK_TYPE getTaskType() ;
         INT64 getTaskID() ;
         const CHAR* getTaskName() ;
         OMA_TASK_STATUS getTaskStatus () ;
         void setTaskStatus( OMA_TASK_STATUS status ) ;
         INT32 setSubTaskStatus( string &name, OMA_TASK_STATUS status ) ;
         INT32 getSubTaskStatus( string &name, OMA_TASK_STATUS &status ) ;
         INT32 getSubTaskSerialNum() ;
         void setJobInfo( EDUID eduID ) ;
         EDUID getJobInfo() ;

      public:
         virtual INT32 initJsEnv() ;

      public:
         virtual INT32 init( const BSONObj &info, void *ptr = NULL ) = 0 ;
         virtual INT32 doit() = 0 ;

      protected:
         ossSpinSLatch                    _latch ;
         OMA_TASK_TYPE                    _taskType ;
         INT64                            _taskID ;
         string                           _taskName ;
         OMA_TASK_STATUS                  _taskStatus ;
         map< string, OMA_TASK_STATUS >   _subTaskStatus ;
         INT32                            _subTaskSerialNum ;
         EDUID                            _eduID ;
   } ;
   typedef _omaTask omaTask ;

   typedef boost::shared_ptr< omaTask >  omaTaskPtr ;

   /*
      task manager
   */
   class _omaTaskMgr : public SDBObject
   {
      public:
         _omaTaskMgr ( INT64 taskID = OMA_INVALID_TASKID ) ;
         ~_omaTaskMgr () ;

         INT64      getTaskID () ;
         void       setTaskID ( INT64 taskID ) ;

      public:
         INT32      addTask ( _omaTask *pTask,
                              INT64 taskID = OMA_INVALID_TASKID ) ;
         INT32      removeTask ( _omaTask *pTask ) ;
         INT32      removeTask ( INT64 taskID ) ;
         INT32      removeTask ( const CHAR *pTaskName ) ;
         _omaTask*  findTask ( INT64 taskID ) ;

      private:
         map<INT64, _omaTask*>        _taskMap ;
         ossSpinSLatch                _taskLatch ;
         INT64                        _taskID ;
   } ;
   typedef _omaTaskMgr omaTaskMgr ;

   /*
      get task manager
   */
   _omaTaskMgr* getTaskMgr() ;

}


#endif  // OMAGENTTASKBASE_HPP_