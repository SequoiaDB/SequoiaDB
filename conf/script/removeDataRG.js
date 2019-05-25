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
@description: remove data group
@modify list:
   2014-7-26 Zhaobo Tan  Init
@parameter
   BUS_JSON: the format is: { "AuthUser": "", "AuthPasswd": "", "UninstallGroupNames": ["group1", "group2"] } ;
   SYS_JSON: the format is: { "TaskID": 1, "TmpCoordSvcName": "10000" } ;
@return
   RET_JSON: the format is: { "errrno": 0, "detail": "" }
*/

var FILE_NAME_REMOVE_DATA_RG = "removeDataRG.js" ;
var RET_JSON = new removeRGResult() ;
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
   // 1. get task id
   task_id = getTaskID( SYS_JSON ) ;
   setTaskLogFileName( task_id ) ;
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_REMOVE_DATA_RG,
            sprintf( "Begin to run task[?] for removing data groups", task_id ) ) ;
}

/* *****************************************************************************
@discretion: final
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _final()
{
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_REMOVE_DATA_RG,
            sprintf( "Finish running task[?] for removing data groups", task_id ) ) ;
}

function main()
{
   var tmpCoordHostName = null ;
   var tmpCoordSvcName  = null ;
   var authUser         = null ;
   var authPasswd       = null ;
   var db               = null ;
   var groupNames       = [] ;
   var csNum            = 0;
   var i                = 0 ;
   
   _init() ;
   
   try
   {
      // 1. get arguments
      try
      {
         tmpCoordHostName = System.getHostName() ;
         tmpCoordSvcName  = SYS_JSON[TmpCoordSvcName] ;
         authUser         = BUS_JSON[AuthUser] ;
         authPasswd       = BUS_JSON[AuthPasswd] ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = "Js receive invalid argument" ;
         rc = GETLASTERROR() ;
         // record error message in log
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_DATA_RG,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         // tell to user error happen
         exception_handle( SDB_INVALIDARG, errMsg ) ;
      }
      // 2. connect to temporary coord
      try
      {
         db = new Sdb ( tmpCoordHostName, tmpCoordSvcName, authUser, authPasswd ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = sprintf( "Failed to connect to temporary coord[?:?]",
                           tmpCoordHostName, tmpCoordSvcName ) ;
         rc = GETLASTERROR() ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_DATA_RG,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         exception_handle( rc, errMsg ) ;
      }
      // 3. check whether the data groups are empty or not
      csNum = db.listCollectionSpaces().size();
      if (csNum != 0)
      {
         rc = SDB_CAT_RM_GRP_FORBIDDEN;
         errMsg = sprintf( "Can't remove data groups, for there still has " + 
                           "[?] collection space(s) in the database", csNum ) ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_DATA_RG,
                  errMsg + ", rc: " + rc ) ;
         exception_handle( rc, errMsg ) ;
      }
      // 4. get data groups from catalog
      try
      {
         var cur = db.listReplicaGroups();
         var ret = null;
         while( (ret = cur.next()) != undefined )
         {
            var rg = JSON.parse( ret ) ;
            if ( rg[GroupName] != OMA_SYS_CATALOG_RG && rg[GroupName] != OMA_SYS_COORD_RG )
            {
               groupNames.push( rg[GroupName] ) ;
            }
         }
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = "Failed to get data group's info from catalog" ;
         rc = GETLASTERROR() ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_DATA_RG,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         exception_handle( rc, errMsg ) ;
      }
      // 5. remove data group
      for ( i = 0; i < groupNames.length; i++ )
      {
         try
         {
            PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_REMOVE_DATA_RG,
                     sprintf( "Removing data group[?]", groupNames[i] ) ) ;
            var j = 0 ;
            for ( ; j < OMA_WAIT_CATALOG_TRY_TIMES; j++ )
            {
               try
               {
                  db.removeRG( groupNames[i] ) ;
               }
               catch( e )
               {
                  if ( SDB_CLS_NOT_PRIMARY == e )
                  {
                     PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_REMOVE_DATA_RG,
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
            if ( OMA_WAIT_CATALOG_TRY_TIMES == j )
            {
               PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_DATA_RG,
                        sprintf( "Data group[?] has no primary", groupNames[i] ) ) ;
               throw SDB_CLS_NOT_PRIMARY ;
            }
         }
         catch( e )
         {
            if ( SDB_CLS_GRP_NOT_EXIST == e )
            {
               continue ;
            }
            else
            {
               SYSEXPHANDLE( e ) ;
               errMsg = sprintf( "Failed to remove data group[?]", groupNames[i] ) ;
               rc = GETLASTERROR() ;
               PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_DATA_RG,
                        errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
               exception_handle( rc, errMsg ) ;
            }
         }
      }

   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = GETLASTERRMSG() ; 
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_DATA_RG,
               sprintf( "Failed to remove all the data group, rc:?, detail:?",
                        rc, errMsg ) ) ;          
      RET_JSON[Errno] = rc ;
      RET_JSON[Detail] = errMsg ;
   }

   _final() ;
   return RET_JSON ;
}

// execute
   main() ;

