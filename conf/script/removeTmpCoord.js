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
@description: remove temporary coord
@modify list:
   2014-7-26 Zhaobo Tan  Init
@parameter
   BUS_JSON: the format is: { "TmpCoordSvcName": "10000" } ;
   SYS_JSON: the format is: { "TaskID": 5 } ;
   ENV_JSON:
@return
   RET_JSON: the format is: {}
*/

var FILE_NAME_REMOVE_TEMPORARY_COORD = "removeTmpCoord.js" ;
var RET_JSON = new removeTmpCoordResult() ;
var rc       = SDB_OK ;
var errMsg   = "" ;

var task_id  = "" ;
var tmp_coord_install_path = "" ;
var tmp_coord_backup_path  = "" ;

/* *****************************************************************************
@discretion: init
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _init()
{
   // get task id
   task_id = getTaskID( SYS_JSON ) ;
   setTaskLogFileName( task_id ) ;
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_REMOVE_TEMPORARY_COORD,
            sprintf( "Begin to remove temporary coord in task[?]", task_id ) ) ;
}

/* *****************************************************************************
@discretion: final
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _final()
{
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_REMOVE_TEMPORARY_COORD,
            sprintf( "Finish removing temporary coord in task[?]", task_id ) ) ;
}

/* *****************************************************************************
@discretion: backup temporary coord's diaglog
@parameter
   svcName[string]: install svc name
@return void
***************************************************************************** */
function _backupTmpCoordDiaglog( svcName )
{
   var src = tmp_coord_install_path + "/diaglog/sdbdiag.log" ;
   var dst = tmp_coord_backup_path + "/diaglog/sdbdiag.log" + "." + genTimeStamp() ;
   PD_LOG2( task_id, arguments, PDDEBUG, FILE_NAME_REMOVE_TEMPORARY_COORD,
            sprintf( "Backup temporary coord's diaglog, src[?], dst[?]", src, dst ) ) ;
   // mkdir director
   File.mkdir( tmp_coord_backup_path + "/diaglog/" ) ;
   // backup sdbdiag.log
   File.copy( src, dst ) ;
}

function main()
{
   var omaHostName      = null ;
   var omaSvcName       = null ;
   var tmpCoordHostName = null ;
   var tmpCoordSvcName  = null ;
   var installInfoObj   = null ;
   var dbInstallPath    = null ;
   var oma              = null ;
   
   _init() ;
   
   try
   {
      // 1. get install temporary coord arguments
      try
      {
         omaHostName    = System.getHostName() ;
         omaSvcName     = Oma.getAOmaSvcName( "localhost" ) ;
         tmpCoordHostName = omaHostName ;
         tmpCoordSvcName  = BUS_JSON[TmpCoordSvcName] ;
         installInfoObj  = eval( '(' + Oma.getOmaInstallInfo() + ')' ) ;
         dbInstallPath   = adaptPath( installInfoObj[INSTALL_DIR] ) ;
         tmp_coord_install_path = dbInstallPath + "database/tmpCoord/" + tmpCoordSvcName ;
         tmp_coord_backup_path  = dbInstallPath + "database/tmpCoordBackup/" + tmpCoordSvcName ;
         
         if ( "undefined" == typeof(tmpCoordSvcName) ||
              "" == tmpCoordSvcName )
            exception_handle( SDB_INVALIDARG, sprintf( "Invalid temporary coord service name[?]", tmpCoordSvcName ) ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to get arguments for removing temporary coord" ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_TEMPORARY_COORD,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
      
      // 2. connet to OM Agent in local host
      try
      {
         oma = new Oma( omaHostName, omaSvcName ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to connect to OM Agent in local host" ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_TEMPORARY_COORD,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
      
      // 3. backup temporary coord's dialog
      try
      {
         _backupTmpCoordDiaglog( tmpCoordSvcName ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to backup temporary coord's diaglog in local host" ;
         PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_REMOVE_TEMPORARY_COORD,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }

      // 4. stop temporary coord
      try
      {
         oma.stopNode( tmpCoordSvcName ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to stop temporary coord" ;
         PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_REMOVE_TEMPORARY_COORD,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }

      // 5. remomve temporary coord
      try
      {
         oma.removeCoord( tmpCoordSvcName ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to remove temporary coord" ;
         PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_REMOVE_TEMPORARY_COORD,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
   
      // 6. close connection
      oma.close() ;
      oma = null ;
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = "Failed to remove temporary coord in local host" ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_TEMPORARY_COORD,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      if ( null != oma && "undefined" != typeof(oma) )
      {
         try
         {
            oma.close() ;
            oma = null ;
         }
         catch ( e1 )
         {
         }
      }
      RET_JSON[Errno]  = rc ;
      RET_JSON[Detail] = errMsg ;
   }
   
   _final() ;
   return RET_JSON ;
}

// execute
   main() ;

