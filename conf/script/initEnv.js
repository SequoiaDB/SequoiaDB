/*******************************************************************************

   Copyright (C) 2012-2014 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*******************************************************************************/
/*
@description: init environment for execute js script
@modify list:
   2014-7-26 Zhaobo Tan  Init
@parameter
   BUS_JSON: the format is: {} ;
   SYS_JSON: the format is: { "TaskID": 3 } ;
   ENV_JSON:
@return
   RET_JSON: the format is: { "errno":0, "detail":"" }
*/

var FILE_NAME_INIT_ENV = "initEnv.js" ;
var RET_JSON = new commonResult() ;
var rc       = SDB_OK ;
var errMsg   = "" ;
var task_id  = "" ;

/* *****************************************************************************
@discretion: init
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _init()
{
   task_id = getTaskID( SYS_JSON ) ;   
   PD_LOG( arguments, PDEVENT, FILE_NAME_INIT_ENV,
           sprintf( "Begin to init environment for executing js script" +
                    " in task[?]", task_id ) ) ;
}

/* *****************************************************************************
@discretion: final
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _final()
{
   PD_LOG( arguments, PDEVENT, FILE_NAME_INIT_ENV,
           sprintf( "Finish initializing environment for executing js script" +
                    " in task[?]", task_id ) ) ;
}

/* *****************************************************************************
@discretion: remove outdated task log file
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _removeTaskLogFile()
{
   var task_log_dir = "" ;
   var log_file     = "" ;
   
   try
   {
      task_log_dir = LOG_FILE_PATH + Task ;
      log_file = adaptPath( task_log_dir ) + task_id + ".log" ;
      if ( true == File.exist( log_file ) )
         File.remove( log_file ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc     = GETLASTERROR() ;
      errMsg = sprintf( "Failed to remove outdated task log file in task[?]", task_id ) ;
      PD_LOG( arguments, PDERROR, FILE_NAME_INIT_ENV,
              sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
   }
}

function main()
{
   _init() ;
   
   try
   {
      // remove outdated task log file
      _removeTaskLogFile() ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = GETLASTERRMSG() ;
      rc = GETLASTERROR() ;
      RET_JSON[Errno] = rc ;
      RET_JSON[Detail] = errMsg ;
   }

   _final() ;
   return RET_JSON ;
}

// execute
   main() ;
