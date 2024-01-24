/*******************************************************************************

   Copyright (C) 2012-2018 SequoiaDB Ltd.

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
@description: execuate psql command
@modify list:
   2016-1-29 YouBin Lin  Init
@parameter
   BUS_JSON: the format is: { "HostName":"suse-lyb", "ServiceName":"5432", "User": "sdbadmin", "Passwd": "sdbadmin", "InstallPath":"/opt/sequoiasql/", "DbName":"postgres", "DbUser":"sdbadmin", "DbPasswd":"sdbadmin", "Sql":"select * from test1", "ResultFormat":"pretty" } ;
   SYS_JSON: the format is: { "TaskID": 1 } ;
   ENV_JSON:
@return
   RET_JSON: the format is: { "errno":0, "detail":"", "PipeFile":"/tmp/omagent/ssql/$TaskID/result.fifo" }
*/

var RET_JSON = new psqlResult() ;
var rc       = SDB_OK ;
var errMsg   = "" ;

var task_id   = "" ;
var host_name = "" ;
var host_svc  = "" ;
var db_name   = "" ;
var db_user   = "" ;
var db_passwd = "" ;
var sql       = "" ;
var format    = "" ;
var FILE_NAME_RUNPSQL = "runPsql.js" ;


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
      format    = BUS_JSON[ResultFormat] ;
      
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
      if ( null == format || undefined == format )
      {
          format = "" ;
      } 
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Js receive invalid argument" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_RUNPSQL,
              sprintf( errMsg + ", rc: ?, detail: ?", GETLASTERROR(), GETLASTERRMSG() ) ) ;
      exception_handle( SDB_INVALIDARG, errMsg ) ;
   }
   
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_RUNPSQL,
            sprintf( "Begin to run psql[?:?:?:?:?:?] in task[?]",
                     host_name, host_svc, db_name, db_user, sql, format, task_id ) ) ;
}

/* *****************************************************************************
@discretion: final
@author: YouBin Lin
@parameter void
@return void
***************************************************************************** */
function _final()
{  
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_RUNPSQL,
            sprintf( "finish run psql[?:?:?:?:?:?] in task[?]",
                     host_name, host_svc, db_name, db_user, sql, format, task_id ) ) ;
}

function main()
{
   var cmd           = null ;
   var exeName       = "" ;
   var argHostName   = "" ;
   var argSvcName    = "" ;
   var argDbName     = "" ;
   var argDbUser     = "" ;
   var argSql        = "" ;
   var argFormat     = "" ;
   var argResultFile = "" ;
   var totalCmd      = "" ;
   var result_file   = "" ;
   var pid_file      = "" ;
   var totalCmd      = "" ;
   _init() ;

   try
   {
      var isPsql = false ;
      var local_lib_path = getPsqlLibPath( System.getEWD() ) ;
      var prelib = 'export LD_LIBRARY_PATH=' + local_lib_path + ':' + '$LD_LIBRARY_PATH'
      createPsqlTaskWorkPath( task_id ) ;

      exeName = getPsqlFile( System.getEWD() ) ;
      argHostName = " -h " + host_name ;
      argSvcName  = " -p " + host_svc ;
      argDbName   = " -d " + db_name ;
      argDbUser   = " -U " + db_user ;
      argDbSql    = " -c " + '"' + sql + '"' ;
      if ( null != format && undefined != format && format == FormatPretty )
      {
         argFormat = " -P 'border=2' " ;
      }

      result_file = createPsqlResultFilePath( task_id ) ;
      pid_file    = createPsqlPidFilePath( task_id ) ;

      argResultFile = " > " + result_file + " 2>&1 ";
      cmd = new Cmd() ;

      totalCmd = prelib + " ; " + exeName + argHostName + argSvcName + argDbName + 
                 argDbUser + argDbSql + argFormat + argResultFile ;
      PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_RUNPSQL,
               sprintf( "start to run psql[?:?:?]",
               host_name, host_svc, totalCmd ) ) ;
      var pid = cmd.start(totalCmd, '', 1, 0) ;

      isPsql = isPsqlProc( pid ) ;
      if ( !isPsql )
      {
         rc = -1 ;
         errMsg = "failed to check pid:" + pid ;
         exception_handle( rc, errMsg ) ;
      }

      try
      {
         PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_RUNPSQL,
                  "start to write pid[" + pid + "] to pid_file[" + pid_file + "]" ) ;
         writeSsqlPidFile( pid_file, pid ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = GETLASTERRMSG() ;
         rc = GETLASTERROR() ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_RUNPSQL,
                  "failed to write pid[" + pid + "] to pid_file[" + pid_file + "]" 
                  + "rc=" + rc + ",detail=" + errMsg ) ;
         try
         {
            var tmpCmd = new Cmd() ;
            tmpCmd.run('kill ' + pid ) ;
         }
         catch( e )
         {
            //do nothing here
         }

         exception_handle( rc, errMsg ) ;
      }

      RET_JSON[Errno]    = rc ;
      RET_JSON[Detail]   = "" ;
      RET_JSON[PipeFile] = result_file ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = GETLASTERRMSG() ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_RUNPSQL,
               sprintf( "Failed to run psql[?:?:?], rc:?, detail:?",
               host_name, host_svc, totalCmd, rc, errMsg ) ) ;
      RET_JSON[Errno] = rc ;
      RET_JSON[Detail] = errMsg ;
   }
   
   _final() ;
   println( JSON.stringify(RET_JSON) ) ;
   return RET_JSON ;
}

// execute
   main() ;
