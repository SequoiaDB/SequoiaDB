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

   Source File Name = omagentSubTask.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/30/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENTSUBTASK_HPP_
#define OMAGENTSUBTASK_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossLatch.hpp"
#include "../bson/bson.h"
#include "omagent.hpp"
#include "omagentTaskBase.hpp"
#include "omagentTask.hpp"
#include <map>
#include <vector>
#include <string>

using namespace std ;
using namespace bson ;

#define OMA_TASK_NAME_ADD_HOST_SUB                    "add host sub task"
#define OMA_TASK_NAME_INSTALL_DB_BUSINESS_SUB         "install db business sub task"
#define OMA_TASK_NAME_INSTALL_ZN_BUSINESS_SUB         "install zookeeper business sub task"
#define OMA_TASK_NAME_INSTALL_SSQL_OLAP_BUSINESS_SUB  "install sequoiasql olap business sub task"

namespace engine
{
   /*
      add host sub task
   */
   class _omaAddHostSubTask : public _omaTask
   {
      public:
         _omaAddHostSubTask ( INT64 taskID ) ;
         virtual ~_omaAddHostSubTask () ;

/*
      public:
         virtual OMA_TASK_TYPE taskType () const { return _taskType ; }
         virtual const CHAR*   taskName () const { return _taskName.c_str() ; }
*/
      public:
         INT32 init( const BSONObj &info, void *ptr = NULL ) ;
         INT32 doit() ;

/*
      public:
         void setTaskStatus( OMA_TASK_STATUS status ) ;
*/

      private:
//         string          _taskName ;
//         OMA_TASK_TYPE   _taskType ;

         _omaAddHostTask    *_pTask ;
         
//         ossSpinSLatch   _taskLatch ;

//         CHAR            _detail[OMA_BUFF_SIZE + 1] ; 
   } ;
   typedef _omaAddHostSubTask omaAddHostSubTask ;

   /*
      add business sub task
   */
   class _omaInstDBBusSubTask : public _omaTask
   {
      public:
         _omaInstDBBusSubTask( INT64 taskID ) ;
         virtual ~_omaInstDBBusSubTask() ;

      public:
         INT32 init( const BSONObj &info, void *ptr = NULL ) ;
         INT32 doit() ;

      private:
         _omaInstDBBusTask    *_pTask ;
   } ;
   typedef _omaAddHostSubTask omaAddHostSubTask ;


   /*
      add zookeeper sub task
   */
   class _omaInstZNBusSubTask : public _omaTask
   {
      public:
         _omaInstZNBusSubTask( INT64 taskID ) ;
         virtual ~_omaInstZNBusSubTask() ;

      public:
         INT32 init( const BSONObj &info, void *ptr = NULL ) ;
         INT32 doit() ;

      private:
         _omaInstZNBusTask    *_pTask ;
   } ;
   typedef _omaInstZNBusSubTask omaInstZNBusSubTask ;

   class _omaInstallSsqlOlapBusSubTask: public _omaTask
   {
      public:
         _omaInstallSsqlOlapBusSubTask( INT64 taskID ) ;
         virtual ~_omaInstallSsqlOlapBusSubTask() ;

      public:
         INT32 init( const BSONObj &info, void *ptr = NULL ) ;
         INT32 doit() ;

      private:
         _omaInstallSsqlOlapBusTask* _mainTask ;
   } ;
}


#endif  //  OMAGENTSUBTASK_HPP_