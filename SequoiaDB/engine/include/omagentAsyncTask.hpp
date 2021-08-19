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

   Source File Name = omagentBackgroundCmd.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/09/2017  HJW Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_ASYNC_TASK_HPP_
#define OMAGENT_ASYNC_TASK_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossLatch.hpp"
#include "ossEvent.hpp"
#include "omagentTaskBase.hpp"

namespace engine
{

   class _omaAsyncTask : public _omaTask
   {
   public:
      _omaAsyncTask( INT64 taskID, const CHAR* command ) ;
      virtual ~_omaAsyncTask() ;
      INT32 init( const BSONObj &info, void *ptr = NULL ) ;
      INT32 doit() ;

   public:
      INT32 setTaskRunning( _omaCommand* cmd, const BSONObj& itemInfo ) ;
      INT32 updateTaskInfo( _omaCommand* cmd, const BSONObj& itemInfo ) ;
      void notifyUpdateProgress() ;
      INT32 updateProgressToOM() ;
      _omaCommand* createOmaCmd() ;
      void deleteOmaCmd( _omaCommand* cmd ) ;
      const CHAR* getOmaCmdName() ;

      BOOLEAN getSubTaskArg( UINT32& planTaskIndex, BSONObj& subTaskArg ) ;

      void setPlanTaskStatus( OMA_TASK_STATUS status ) ;
      OMA_TASK_STATUS getPlanTaskStatus() ;

   private:
      INT32 _generateTaskPlan( BSONObj& planInfo ) ;
      INT32 _execTaskPlan( const BSONObj& planInfo ) ;
      void _setTaskResultInfoStatus( OMA_TASK_STATUS status ) ;
      INT32 _rollbackPlan() ;

      INT32 _createSubTask() ;
      INT32 _waitSubTask() ;

      void _initSubTaskArg() ;
      void _appendSubTaskArg( BSONObj& subTaskArg ) ;

   private:
      INT32           _errno ;
      BOOLEAN         _isSetErrInfo ;
      string          _command ;
      string          _detail ;
      BSONObj         _taskInfo ;

      UINT32          _planTaskIndex ;
      UINT64          _planTaskNum ;
      ossEvent        _planEvent ;
      ossSpinSLatch   _planLatch ;
      vector<BSONObj> _planTaskArgList ;
      //vector<BSONObj> _planTaskResultList ;
   } ;
   typedef _omaAsyncTask omaAsyncTask ;

   class _omaAsyncSubTask : public _omaTask
   {
   public:
      _omaAsyncSubTask( INT64 taskID ) ;
      virtual ~_omaAsyncSubTask() ;

   public:
      INT32 init( const BSONObj& info, void* ptr = NULL ) ;
      INT32 doit() ;

   private:
      _omaAsyncTask* _task ;
      BSONObj        _taskInfo ;
   } ;
   typedef _omaAsyncSubTask omaAsyncSubTask ;

}

#endif