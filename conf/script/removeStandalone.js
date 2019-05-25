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
@description: remove standalone
@modify list:
   2014-7-26 Zhaobo Tan  Init
@parameter
   BUS_JSON: the format is: { "UninstallHostName": "susetzb", "UninstallSvcName": "20000" } ;
   SYS_JSON: the format is: { "TaskID": 4 } ;
@return
   RET_JSON: the format is: { "errno": 0, "detail": "" }
*/

var FILE_NAME_REMOVE_STANDALONE = "removeStandalone.js" ;
var RET_JSON = new removeNodeResult() ;
var rc       = SDB_OK ;
var errMsg   = "" ;

var task_id = "" ;

var host_name = "" ;
var host_svc  = "" ;

/* *****************************************************************************
@discretion: init
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _init()
{
   // 1. get task id
   task_id = getTaskID( SYS_JSON ) ;

   // 2. specify the log file name
   try
   {
      host_name = BUS_JSON[UninstallHostName] ;
      host_svc  = BUS_JSON[UninstallSvcName] ;
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Js receive invalid argument" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_REMOVE_STANDALONE,
              sprintf( errMsg + ", rc: ?, detail: ?", GETLASTERROR(), GETLASTERRMSG() ) ) ;
      exception_handle( SDB_SYS, errMsg ) ;
   }
   
   setTaskLogFileName( task_id ) ;
   
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_REMOVE_STANDALONE,
            sprintf( "Begin to remove standalone[?:?] in task[?]",
                     host_name, host_svc, task_id ) ) ;
}

/* *****************************************************************************
@discretion: final
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _final()
{
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_REMOVE_STANDALONE,
            sprintf( "Finish removing standalone[?:?] in task[?]",
                     host_name, host_svc, task_id ) ) ;
}


/* *****************************************************************************
@discretion remove standalone
@parameter
   hostName[string]: uninstall host name
   svcName[string]: uninstall svc name
   agentPort[string]: the port of sdbcm in standalone host
@return void
***************************************************************************** */
function _removeStandalone( hostName, svcName, agentPort )
{
   var oma = null ;
   try
   {
      oma = new Oma( hostName, agentPort ) ;
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = sprintf( "Failed to connect to OM Agent[?:?] in local host",
                        hostName, agentPort ) ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_STANDALONE,
               sprintf( errMsg + ", rc:?, detail:?", rc, errMsg ) ) ;  
      exception_handle( rc, errMsg ) ;
   }
   // remove standalone
   try
   {
      oma.removeData( svcName ) ;
   }
   catch( e )
   {
      if ( SDBCM_NODE_NOTEXISTED != e )
         exception_handle( GETLASTERROR(), GETLASTERRMSG() ) ;
   }
   try
   {
      oma.close() ;
      oma = null ;
   }
   catch ( e ) 
   {
   }
}

function main()
{
   var uninstallHostName  = null ;
   var uninstallSvcName   = null ;
   var agentPort          = null ;
   
   _init() ;
   
   try
   {
      // 1. get arguments
      try
      {
         uninstallHostName = BUS_JSON[UninstallHostName] ;
         uninstallSvcName  = BUS_JSON[UninstallSvcName] ;
         agentPort         = getOMASvcFromCfgFile( uninstallHostName ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = "Js receive invalid argument" ;
         rc = GETLASTERROR() ;
         // record error message in log
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_STANDALONE,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         // tell to user error happen
         exception_handle( SDB_INVALIDARG, errMsg ) ;
      }
      // 2. remove standalone
      _removeStandalone( uninstallHostName, uninstallSvcName, agentPort ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = GETLASTERRMSG() ; 
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_STANDALONE,
               sprintf( "Failed to remove standalone[?:?], rc:?, detail:?",
               host_name, host_svc, rc, errMsg ) ) ;             
      RET_JSON[Errno] = rc ;
      RET_JSON[Detail] = errMsg ;
   }
   
   _final() ;
   return RET_JSON ;
}

// execute
   main() ;

