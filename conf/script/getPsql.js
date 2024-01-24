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
@description: get psql's process and lib
@modify list:
   2016-1-29 YouBin Lin  Init
@parameter
   BUS_JSON: the format is: { "HostName":"suse-lyb", "ServiceName":"5432", "User": "sdbadmin", "Passwd": "sdbadmin", "InstallPath":"/opt/sequoiasql/", "DbName":"postgres", "DbUser":"sdbadmin", "DbPasswd":"sdbadmin", "Sql":"select * from test1", "ResultFormat":"pretty" } ;
   SYS_JSON: the format is: { "TaskID": 1 } ;
   ENV_JSON:
@return
   RET_JSON: the format is: { "errno":0, "detail":"" }
*/

var RET_JSON  = new commonResult() ;
var rc        = SDB_OK ;
var errMsg    = "" ;
var task_id   = "" ;
var host_name = "" ;
var ssh_port  = "22" ;
var user      = "" ;
var passwd    = "" ;
var dir_sep   = "" ;
var install_path = "" ;


var FILE_NAME_GETPSQL = "getPsql.js" ;


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
      user      = BUS_JSON[User] ;
      passwd    = BUS_JSON[Passwd] ;
      install_path = BUS_JSON[InstallPath] ;
      
      if ( null == host_name || undefined == host_name )
      {
         exception_handle( SDB_INVALIDARG, "BUS_JSON[HostName] is not set" ) ; 
      }
      if ( null == user || undefined == user )
      {
         exception_handle( SDB_INVALIDARG, "BUS_JSON[User] is not set" ) ; 
      }
      if ( null == passwd || undefined == passwd )
      {
         exception_handle( SDB_INVALIDARG, "BUS_JSON[Passwd] is not set" ) ; 
      }
      if ( null == install_path || undefined == install_path )
      {
         exception_handle( SDB_INVALIDARG, "BUS_JSON[InstallPath] is not set" ) ; 
      }
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Js receive invalid argument" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_GETPSQL,
              sprintf( errMsg + ", rc: ?, detail: ?", GETLASTERROR(), GETLASTERRMSG() ) ) ;
      exception_handle( SDB_INVALIDARG, errMsg ) ;
   }
   
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_GETPSQL,
            sprintf( "Begin to get psql[?:?:?:?] in task[?]",
                     host_name, ssh_port, user, install_path, task_id ) ) ;
}

/* *****************************************************************************
@discretion: pullFileFromRemote
@author: YouBin Lin
@parameter 
ssh:            ssh
remoteLibDir:   remote host's lib directory
remoteLibArray: remote host's library array
localLibDir:    local host's lib directory
@return void
***************************************************************************** */
function pullFileFromRemote( ssh, remoteLibDir, remoteLibArray, localLibDir )
{
   var cmd      = new Cmd() ;
   var chmodCmd = 'chmod a+x ' ;
   for ( var i = 0; i < remoteLibArray.length; i++ )
   {
      var remote_lib = remoteLibDir + dir_sep + remoteLibArray[i] ;
      var local_lib  = localLibDir + dir_sep + remoteLibArray[i] ;
      
      PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_GETPSQL,
            sprintf( "get file:remote[?],local[?]", remote_lib,
            local_lib ) ) ;
      ssh.pull(remote_lib, local_lib) ;
      cmd.run( chmodCmd + local_lib ) ;
   }
}

/* *****************************************************************************
@discretion: final
@author: YouBin Lin
@parameter void
@return void
***************************************************************************** */
function _final()
{  
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_GETPSQL,
            sprintf( "finish get psql[?:?:?:?] in task[?]",
                     host_name, ssh_port, user, install_path, task_id ) ) ;
}

function main()
{
   var ssh            = null ;
   var remote_bin     = "" ;
   var local_bin      = "" ;
   var local_lib_path = "" ;

   if ( SYS_LINUX == SYS_TYPE )
   {
      dir_sep = "/" ;
   }
   else
   {
      dir_sep = "\\" ;
   }

   _init() ;

   try
   {
      var isLocalBinExist = false ;
      var remoteLibpqPath = adaptPath(install_path) + "lib" + dir_sep ;
      //var libPqArray      = new Array('libpq.so*');
      var libPqArray      = new Array('libpq.so', 'libpq.so.5', 'libpq.so.5.6');

 //     var remoteLibeditPath = adaptPath(install_path) + "thirdparty" + dir_sep + "lib" + dir_sep ;
      //var libEditArray      = new Array('libedit.so', 'libedit.so.0', 'libedit.so.0.0.53');
      
      remote_bin     = adaptPath(install_path) + "bin" + dir_sep + "psql" ;
      local_bin      = getPsqlFile( System.getEWD() ) ;
      local_lib_path = getPsqlLibPath( System.getEWD() ) ;

      isLocalBinExist = File.exist( local_bin ) ;
      if ( isLocalBinExist == false )
      {
         var chmodCmd = 'chmod a+x ' ;
         var cmd      = null ;
         File.mkdir( local_lib_path ) ;
         
         PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_GETPSQL,
                  sprintf( "start to ssh:remote[?:?]", host_name, user ) ) ;
         ssh = new Ssh( host_name, user, passwd ) ;
         
         pullFileFromRemote( ssh, remoteLibpqPath, libPqArray, local_lib_path ) ;
//        pullFileFromRemote( ssh, remoteLibeditPath, libEditArray, local_lib_path ) ;

         PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_GETPSQL,
                  sprintf( "get file:remote[?],local[?]", remote_bin,
                  local_bin ) ) ;
         ssh.pull(remote_bin, local_bin) ;
         
         cmd = new Cmd() ;
         cmd.run( chmodCmd + local_bin ) ;
      }

      RET_JSON[Errno]    = rc ;
      RET_JSON[Detail]   = "" ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = GETLASTERRMSG() ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_GETPSQL,
               sprintf( "Failed to get psql[?:?], rc:?, detail:?",
               host_name, ssh_port, rc, errMsg ) ) ;
      RET_JSON[Errno]  = rc ;
      RET_JSON[Detail] = errMsg ;
   }

   _final() ;
   println( JSON.stringify(RET_JSON) ) ;
   return RET_JSON ;
}

// execute
   main() ;
