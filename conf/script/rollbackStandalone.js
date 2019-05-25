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
@description: rollback standalone
@modify list:
   2014-7-26 Zhaobo Tan  Init
@parameter
   BUS_JSON: the format is: { "UninstallHostName": "susetzb", "UninstallSvcName": "20000" } ;
   SYS_JSON: the format is: { "TaskID": 3 } ;
@return
   RET_JSON: the format is: { "errno":0, "detail":"" }
*/

var RET_JSON = new rollbackNodeResult() ;
var rc       = SDB_OK ;
var errMsg   = "" ;

var task_id   = "" ;
var host_ip   = "" ;
var host_name = "" ;
var host_svc  = "" ;
var FILE_NAME_ROLLBACKSTANDALONE = "rollbackStandalone.js" ;

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
      host_svc = BUS_JSON[UninstallSvcName] ;
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Js receive invalid argument" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_ROLLBACKSTANDALONE,
              sprintf( errMsg + ", rc: ?, detail: ?", GETLASTERROR(), GETLASTERRMSG() ) ) ;
      exception_handle( SDB_SYS, errMsg ) ;
   }
   
   setTaskLogFileName( task_id ) ;
   
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_ROLLBACKSTANDALONE,
            sprintf( "Begin to rollback standalone[?:?]", host_name, host_svc ) ) ;
}

/* *****************************************************************************
@discretion: final
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _final()
{
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_ROLLBACKSTANDALONE,
            sprintf( "Finish rollbacking standalone[?:?]", host_name, host_svc ) ) ;
}

function _checkData( hostName, svcName )
{
   var hasData = false ;

   try
   {
      var db = new Sdb( hostName, svcName ) ;
      var cursor = db.list( SDB_LIST_COLLECTIONSPACES ) ;
      while( true )
      {
         var json ;
         var record = cursor.next() ;

         if ( record === undefined )
         {
            break ;
         }

         json = record.toObj() ;

         if ( 'string' == typeof( json[FIELD_NAME] ) &&
              json[FIELD_NAME].indexOf( 'SYS' ) != 0 )
         {
            hasData = true ;
            break ;
         }
      }
   }
   catch( e )
   {
      PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_INSTALL_STANDALONE,
            sprintf( "Failed to connect standalone[?:?], detail: ?",
                     hostName, svcName, GETLASTERRMSG() ) ) ;
   }

   return hasData ;
}

/* *****************************************************************************
@discretion remove standalone
@parameter
   hostName[string]: remove host name
   svcName[string]: remove service name
   agentPort[string]: the port of sdbcm in target standalone host
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
      rc = GETLASTERROR() ;
      errMsg = sprintf( "Failed to connect to OM Agent in host[?:?]", hostName, agentPort );
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_ROLLBACKSTANDALONE,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   // remove standalone
   try
   {
      oma.removeData( svcName ) ;
   }
   catch ( e ) 
   {
      if ( null != oma && "undefined" != typeof(oma) )
      {
         try
         {
            oma.close() ;
         }
         catch ( e2 )
         {
         }
      }
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = sprintf( "Failed to remove standalone[?:?]", hostName, svcName ) ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_ROLLBACKSTANDALONE,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   // close oma
   try
   {
      oma.close() ;
   }
   catch( e )
   {
   }
}

function main()
{
   var hostName  = null ;
   var svcName   = null ;
   var agentPort = null ;

   _init() ;
   try
   {
      hostName  = BUS_JSON[UninstallHostName] ;
      svcName   = BUS_JSON[UninstallSvcName] ;
      agentPort = getOMASvcFromCfgFile( hostName ) ;
      // uninstall standalone
      if ( true !== _checkData( hostName, svcName ) )
      {
         _removeStandalone( hostName, svcName, agentPort ) ;
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = GETLASTERRMSG() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_ROLLBACKSTANDALONE,
               sprintf( "Failed to rollback standalone[?:?], rc: ?, detail: ?",
                       hostName, svcName, rc, errMsg ) ) ;
      RET_JSON[Errno] = rc ;
      RET_JSON[Detail] = errMsg ;
   }
   
   _final() ;
   return RET_JSON ;
}

// execute
   main() ;

