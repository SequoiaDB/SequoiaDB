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
@description: remove the newly created coord group
@modify list:
   2014-7-26 Zhaobo Tan  Init
@parameter
   BUS_JSON:
   SYS_JSON: the format is: { "TaskID": 2, "TmpCoordSvcName": "10000" } ;
   ENV_JSON:
@return
   RET_JSON: the format is: { "errrno": 0, "detail": "" }
*/

var FILE_NAME_ROLLBACK_COORD = "rollbackCoord.js" ;
var RET_JSON     = new removeRGResult() ;
var rc           = SDB_OK ;
var errMsg       = "" ;

var task_id      = "" ;

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
   setTaskLogFileName( task_id ) ;
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_ROLLBACK_COORD,
            sprintf( "Begin to rollback coord rg in task[?]", task_id ) ) ;
}

/* *****************************************************************************
@discretion: final
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _final()
{
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_ROLLBACK_COORD,
            sprintf( "Finish rollback coord group in task[?]", task_id ) ) ;
}

/* *****************************************************************************
@discretion: remove coord group
@parameter
   db[object]: Sdb object
@return void
***************************************************************************** */
function _removeCoordGroup( db )
{
   var rg = null ;

   try
   {
      // test whether catalog is ok or not
      // when catalog has no primary, wait for a while
      var i = 0 ;
      for ( ; i < OMA_WAIT_CATALOG_TRY_TIMES; i++ )
      {
         try
         {
            rg = db.getCoordRG() ;
         }
         catch( e )
         {
            if ( SDB_CLS_NOT_PRIMARY == e )
            {
               PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_ROLLBACK_COORD,
                        "Catalog has no primary, waiting 1 sec" ) ;
               sleep( 1000 ) ; // l sec
               continue ;
            }
            else
            {
               throw e ;
            }
         }
         break ;
      }
      if ( OMA_WAIT_CATALOG_TRY_TIMES == i )
      {
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_ROLLBACK_COORD,
                  "Catalog has no primary" ) ;
         throw SDB_CLS_NOT_PRIMARY ;
      }
   }
   catch ( e )
   {
      if ( SDB_CLS_GRP_NOT_EXIST == e )
      {
         PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_ROLLBACK_COORD,
                  "No coord group needs to rollback" ) ;
         return ;
      }
      else
      {
         SYSEXPHANDLE( e ) ;
         errMsg = "Failed to get coord group" ;
         rc = GETLASTERROR() ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_ROLLBACK_COORD,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         exception_handle( rc, errMsg ) ;
      }
   }
   // remove
   try
   {
      db.removeCoordRG() ;
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to remove coord group" ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_ROLLBACK_COORD,
               errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
      exception_handle( rc, errMsg ) ;
   }
}

function main()
{
   var tmpCoordHostName   = null ;
   var tmpCoordSvcName    = null ;
   var db                 = null ;
   
   _init() ;
   
   try
   {
      // 1. get arguments
      try
      {
         tmpCoordHostName   = System.getHostName() ;
         tmpCoordSvcName    = SYS_JSON[TmpCoordSvcName] ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = "Js receive invalid argument" ;
         rc = GETLASTERROR() ;
         // record error message in log
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_ROLLBACK_COORD,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         // tell to user error happen
         exception_handle( SDB_INVALIDARG, errMsg ) ;
      }
      
      // 2. connect to temporary coord
      try
      {
         db = new Sdb( tmpCoordHostName, tmpCoordSvcName, "", "" ) ;
      }
      catch ( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = sprintf( "Failed to connect to temporary coord[?:?]",
                           tmpCoordHostName, tmpCoordSvcName ) ;
         rc = GETLASTERROR() ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_ROLLBACK_COORD,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         exception_handle( rc, errMsg ) ;
      }
      
      // 3. test whether catalog is running or not
      // if catalog is not running, no need to rollback
      if ( false == isCatalogRunning( db ) )
      {
         PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_ROLLBACK_COORD,
                  sprintf( "Catalog is not running, stop removing coord group" ) ) ;
         _final() ;
         return RET_JSON ;
      }
      
      // 4. remove coord nodes
      _removeCoordGroup( db ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = GETLASTERRMSG() ; 
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_ROLLBACK_COORD,
               sprintf( "Failed to remove catalog group, rc:?, detail:?",
                        rc, errMsg ) ) ;             
      RET_JSON[Errno] = rc ;
      RET_JSON[Detail] = errMsg ;
   }
   
   _final() ;
   return RET_JSON ;
}

// execute
   main() ;

