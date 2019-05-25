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
@description: clean psql command
@modify list:
   2016-1-29 YouBin Lin  Init
@parameter
   BUS_JSON: the format is: { "HostName":"suse-lyb", "ServiceName":"5432", "User": "sdbadmin", "Passwd": "sdbadmin", "InstallPath":"/opt/sequoiasql/", "DbName":"postgres", "DbUser":"sdbadmin", "DbPasswd":"sdbadmin", "Sql":"select * from test1", "ResultFormat":"pretty" } ;
   SYS_JSON: the format is: { "TaskID": 1 } ;
   ENV_JSON:
@return
   RET_JSON: the format is: { "errno":0, "detail":"" }
*/

var RET_JSON = new cleanPsqlResult() ;
var rc       = SDB_OK ;
var errMsg   = "" ;

var task_id   = "" ;
var host_name = "" ;
var host_svc  = "" ;
var db_name   = "" ;
var db_user   = "" ;
var db_passwd = "" ;
var sql       = "" ;
var FILE_NAME_CLEANPSQL = "cleanPsql.js" ;


/* *****************************************************************************
@discretion: init
@author: YouBin Lin
@parameter void
@return void
***************************************************************************** */
function _init()
{
   // 1. get task id
   task_id = getTaskID( SYS_JSON ) ;
   // 2. specify the log file name
   setTaskLogFileName( task_id ) ;

   try
   {
      host_name = BUS_JSON[HostName] ;
      host_svc  = BUS_JSON[ServiceName] ;
      db_name   = BUS_JSON[DbName] ;
      db_user   = BUS_JSON[DbUser] ;
      db_passwd = BUS_JSON[DbPasswd] ;
      sql       = BUS_JSON[Sql] ;
      
      if ( null == host_name || undefined == host_name )
      {
         exception_handle( SDB_INVALIDARG, "BUS_JSON[HostName] is not set" ) ; 
      }
      if ( null == host_svc || undefined == host_svc )
      {
         exception_handle( SDB_INVALIDARG, "BUS_JSON[ServiceName] is not set" ) ; 
      }
      if ( null == db_name || undefined == db_name )
      {
         exception_handle( SDB_INVALIDARG, "BUS_JSON[DbName] is not set" ) ; 
      }
      if ( null == db_user || undefined == db_user )
      {
         exception_handle( SDB_INVALIDARG, "BUS_JSON[DbUser] is not set" ) ; 
      }
      if ( null == db_passwd || undefined == db_passwd )
      {
         exception_handle( SDB_INVALIDARG, "BUS_JSON[DbPasswd] is not set" ) ; 
      }
      if ( null == sql || undefined == sql )
      {
         exception_handle( SDB_INVALIDARG, "BUS_JSON[Sql] is not set" ) ; 
      }
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Js receive invalid argument" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_CLEANPSQL,
              sprintf( errMsg + ", rc: ?, detail: ?", GETLASTERROR(), GETLASTERRMSG() ) ) ;
      exception_handle( SDB_INVALIDARG, errMsg ) ;
   }
   
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_CLEANPSQL,
            sprintf( "Begin to clean psql[?:?:?:?:?] in task[?]",
                     host_name, host_svc, db_name, db_user, sql, task_id ) ) ;
}

/* *****************************************************************************
@discretion: final
@author: YouBin Lin
@parameter void
@return void
***************************************************************************** */
function _final()
{  
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_CLEANPSQL,
            sprintf( "finish clean psql[?:?:?:?:?] in task[?]",
                     host_name, host_svc, db_name, db_user, sql, task_id ) ) ;
}

function main()
{
   _init() ;

   try
   {
      var isExist = isTaskWorkDirExist( task_id ) ;
      if ( isExist == true )
      {
         var pid = getPidFromPsqlPidFile( task_id ) ;
         if ( null != pid && undefined != pid )
         {
            var cmd = new Cmd() ;
            try
            {
               PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_CLEANPSQL,
                        "start to kill psql process:" + pid ) ;
               cmd.run("ps awx -o \"%p %P\" | grep  " + pid + " | awk ' { print $1} ' | xargs kill" ) ;
            }
            catch( e )
            {
               //ignore if pid is not exist
               var tmpErr = GETLASTERRMSG() ;
               var tmpRc = GETLASTERROR() ;
               PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CLEANPSQL, "run error:rc=" + tmpRc + ",err=" + tmpErr ) ;
            }
         }
         else
         {
            PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_CLEANPSQL,
                     "psql's process is not exist" ) ;
         }
  
         PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_CLEANPSQL,
                  "start to remove pid file" ) ;
         removePsqlPidFile( task_id ) ;
         PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_CLEANPSQL,
                  "start to remove sql result file" ) ;
         removePsqlResultFile( task_id ) ;
         PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_CLEANPSQL,
                  "start to remove task work path" ) ;
         removePsqlTaskWorkPath( task_id ) ;
      }
      else
      {
         PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_CLEANPSQL,
                  "task's work dir is not exist" ) ;
      }

      RET_JSON[Errno]    = rc ;
      RET_JSON[Detail]   = "" ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = GETLASTERRMSG() ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CLEANPSQL,
               sprintf( "Failed to clean psql[?:?], rc:?, detail:?",
               host_name, host_svc, rc, errMsg ) ) ;
      RET_JSON[Errno] = rc ;
      RET_JSON[Detail] = errMsg ;
   }
   
   _final() ;
   println( JSON.stringify(RET_JSON) ) ;
   return RET_JSON ;
}

// execute
   main() ;
